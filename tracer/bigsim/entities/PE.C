//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
//
// Written by:
//     Nikhil Jain <nikhil.jain@acm.org>
//     Bilge Acun <acun2@illinois.edu>
//     Abhinav Bhatele <bhatele@llnl.gov>
//
// LLNL-CODE-740483. All rights reserved.
//
// This file is part of TraceR. For details, see:
// https://github.com/LLNL/TraceR
// Please also read the LICENSE file for the MIT License notice.
//////////////////////////////////////////////////////////////////////////////

#include "assert.h"
#include "PE.h"
#include <math.h>
#define MAX_LOGS 5000000
extern JobInf *jobs;

PE::PE() {
  busy = false;
  currentTask = 0;
  beforeTask = 0;
  currIter = 0;
  loop_start_task = -1; 
}

PE::~PE() {
    msgBuffer.clear();
#if TRACER_BIGSIM_TRACES
    delete [] myTasks;
    delete [] msgDestLogs;
#endif
}

void PE::mark_all_done(int iter, int tInd) {
  if(allMarked[iter]) return;
  for(int i = tInd + 1; i < tasksCount; i++) {
    taskStatus[iter][i] = true;
  }
#if TRACER_OTF_TRACES
  for(int i = 0; i < loop_start_task; i++) {
    taskStatus[iter+1][i] = true;
  }
#endif
  allMarked[iter] =  true;
}

bool PE::noUnsatDep(int iter, int tInd)
{
#if TRACER_BIGSIM_TRACES
  for(int i=0; i<myTasks[tInd].backwDepSize; i++)
  {
    int bwInd = myTasks[tInd].backwardDep[i];
    if(!taskStatus[iter][bwInd])
      return false;
  }
  return true;
#else
  if(tInd != 0) {
    return taskStatus[iter][tInd - 1];
  } else return true;
#endif
}

double PE::taskExecTime(int tInd)
{
  return myTasks[tInd].execTime;
}

void PE::printStat()
{
  int countTask=0;
  for(int j = 0; j < jobs[jobNum].numIters; j++) {
    for(int i=0; i<tasksCount; i++)
    {
      if(!taskStatus[j][i])
      {
        countTask++;
      }
    }
  }
  if(countTask != 0) {
    printf("PE%d: not done count:%d out of %d \n",myNum, countTask, tasksCount);
  }
}

void PE::check()
{
  printStat();
}

void PE::printState()
{
  printStat();
}

void PE::invertMsgPe(int iter, int tInd)
{
  msgStatus[iter][tInd] = !msgStatus[iter][tInd];
}

double PE::getTaskExecTime(int tInd)
{
  return myTasks[tInd].execTime;
}

void PE::addTaskExecTime(int tInd, double time)
{
  myTasks[tInd].execTime += time;
}

int PE::findTaskFromMsg(MsgID* msgId)
{
  std::map<int, int>::iterator it;
  int sPe = msgId->pe;
  int sEmPe = (sPe/numWth)%numEmPes;
  int smsgID = msgId->id;
  it = msgDestLogs[sEmPe].find(smsgID);
  if(it!=msgDestLogs[sEmPe].end())
    return it->second;
  else return -1;
}
