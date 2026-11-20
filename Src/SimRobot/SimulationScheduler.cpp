/**
 * @file SimRobot/SimulationScheduler.cpp
 * Implementation of class SimulationScheduler
 */

#include "SimulationScheduler.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>

SimulationScheduler::SimulationScheduler(std::function<void()> step, std::function<void()> updateGui, std::function<double()> refreshRate, QObject* parent) :
  QObject(parent), step(std::move(step)), updateGui(std::move(updateGui)), refreshRate(std::move(refreshRate))
{
  timer.setTimerType(Qt::PreciseTimer);
  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, this, &SimulationScheduler::slice);
}

void SimulationScheduler::start()
{
  if(running)
    return;
  running = true;
  singleStep = false;
  nextStep = lastGuiUpdate = lastFrame = now();
  waitingForFrame = false;
  schedule(0.);
}

void SimulationScheduler::stop()
{
  if(!running)
    return;
  running = false;
  finalUpdate = true;
  schedule(0.);
}

void SimulationScheduler::stepOnce()
{
  if(running)
  {
    stop();
    return;
  }
  singleStep = finalUpdate = true;
  schedule(0.);
}

void SimulationScheduler::reset()
{
  timer.stop();
  running = singleStep = finalUpdate = waitingForFrame = false;
  stepInterval = 0.;
}

void SimulationScheduler::setStepInterval(double seconds)
{
  stepInterval = std::max(0., seconds * 1000.);
  nextStep = now();
  if(running)
    schedule(0.);
}

void SimulationScheduler::setGuiUpdateInterval(int milliseconds)
{
  guiInterval = std::max(0, milliseconds);
  if(running)
    schedule(0.);
}

void SimulationScheduler::frameFinished()
{
  const double t = now();
  if(!framesExpected)
    lastFrame = t;
  framesExpected = true;
  if(!waitingForFrame)
    return;
  waitingForFrame = false;
  // Presenting waits for the display if the frame was ready early, so that the frames are presented once per period
  // then. If the update starts too late, the frame takes longer than a period (on displays with a fixed refresh rate,
  // it even takes two), so the lead time is increased. As the display cannot tell how early the frame could be ready,
  // the lead time is slowly decreased to find out how little is enough (so a frame is late from time to time).
  const double interval = t - lastFrame;
  excess += (std::clamp(interval - refreshPeriod(), -1., 4.) - excess) * 0.02;
  if(excess > 0.5)
  {
    leadTime += std::min(excess, 1.);
    excess = 0.;
  }
  else
    leadTime = std::max(minLeadTime, leadTime - leadTimeDecay * interval);
  lastFrame = t;
  if(running)
    schedule(0.);
}

double SimulationScheduler::now()
{
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

double SimulationScheduler::refreshPeriod() const
{
  const double rate = refreshRate();
  return 1000. / (rate > 1. ? rate : 60.);
}

double SimulationScheduler::guiDue() const
{
  if(guiInterval > 0)
    return lastGuiUpdate + guiInterval;
  const double period = refreshPeriod();
  if(!framesExpected)
    return lastGuiUpdate + period;
  // The display refreshes one period after the last frame was presented. The update has to be started early enough
  // that the frame is ready by then.
  return lastFrame + period - std::min(leadTime, period * 0.8);
}

void SimulationScheduler::schedule(double time)
{
  const double delay = time - now();
  timer.start(delay > 1. ? static_cast<int>(delay) : 0);
}

void SimulationScheduler::slice()
{
  if(!running)
  {
    // The simulation was stopped or a single step has been requested.
    if(singleStep)
    {
      singleStep = false;
      step();
    }
    if(finalUpdate)
    {
      finalUpdate = false;
      doUpdateGui();
    }
    return;
  }

  double t = now();
  if(waitingForFrame)
  {
    if(t < frameTimeout)
    {
      schedule(frameTimeout);
      return;
    }
    // The frame never arrived, e.g. because the window is hidden.
    waitingForFrame = false;
    framesExpected = false;
  }

  const double period = refreshPeriod();
  const double due = guiDue();
  const double sliceEnd = t + sliceDuration;
  while(running && t < due)
  {
    if(stepInterval > 0. ? nextStep > t : t + std::min(stepCost, period * 0.5) > due)
      break;
    step();
    const double after = now();
    stepCost += (after - t - stepCost) * 0.1;
    t = after;
    if(stepInterval > 0.)
    {
      nextStep += stepInterval;
      if(t - nextStep > maxLag) // The simulation cannot keep up with real time.
        nextStep = t - maxLag;
    }
    if(t >= sliceEnd)
      break;
  }

  if(!running)
  {
    // stop was called by the step.
    slice();
    return;
  }

  if(t >= due)
  {
    doUpdateGui();
    if(waitingForFrame)
    {
      schedule(frameTimeout);
      return;
    }
  }

  const double nextDue = guiDue();
  if(stepInterval > 0.)
    schedule(std::min(nextStep, nextDue));
  else
    schedule(now() + std::min(stepCost, period * 0.5) > nextDue ? nextDue : 0.);
}

void SimulationScheduler::doUpdateGui()
{
  const double t = now();
  lastGuiUpdate = t;
  updateGui();
  // After the update, steps are paused until the frame is presented.
  // That is only expected if presenting has been seen, so that nothing waits for a window that never presents.
  waitingForFrame = running && guiInterval == 0 && framesExpected;
  frameTimeout = t + 2. * refreshPeriod();
}
