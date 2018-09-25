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

#ifndef PE_H_
#define PE_H_

#include "MsgEntry.h"
#include <cstring>
#include "Task.h"
#include <list>
#include <map>
#include <vector>
#include "datatypes.h"

class Task;

class MsgKey {
  public:
  uint32_t rank, comm, tag;
  int64_t seq;
  MsgKey() {
    rank = 0; tag = 0; comm = 0; seq = 0;
  }
  MsgKey(uint32_t _rank, uint32_t _tag, uint32_t _comm, int64_t _seq) {
    rank = _rank; tag = _tag; comm = _comm; seq = _seq;
  }
  bool operator< (const MsgKey &rhs) const {
    if(rank != rhs.rank) return rank < rhs.rank;
    else if(tag != rhs.tag) return tag < rhs.tag;
    else return comm < rhs.comm;
    //else if(comm != rhs.comm) return comm < rhs.comm;
    //else return seq < rhs.seq;
  }
  ~MsgKey() { }
};
typedef std::map< MsgKey, std::list<int> > KeyType;

class CollMsgKey {
  public:
  uint32_t rank, comm;
  int64_t seq;
  CollMsgKey(uint32_t _rank, uint32_t _comm, int64_t _seq) {
    rank = _rank; comm = _comm; seq = _seq;
  }
  bool operator< (const CollMsgKey &rhs) const {
    if(rank != rhs.rank) return rank < rhs.rank;
    else if(comm != rhs.comm) return comm < rhs.comm;
    else return seq < rhs.seq;
  }
  ~CollMsgKey() { }
};
typedef std::map< CollMsgKey, std::list<int> > CollKeyType;

class PE {
  public:
    PE();
    ~PE();
    std::list<TaskPair> msgBuffer;
    Task* myTasks;	// all tasks of this PE
    bool **taskStatus, **taskExecuted;
    bool **msgStatus;
    bool *allMarked;
    double currTime;
    bool busy;
    int beforeTask, totalTasksCount;
    int myNum, myEmPE, jobNum;
    int tasksCount;	//total number of tasks
    int currentTask; // index of first not-executed task (helps searching messages)
    int currentTaskOnTopOfStack; // index of first not-executed task (helps searching messages). Used exclusively in RDMA protocol implementation
    int firstTask;
    int currIter;
    int loop_start_task;

    /*RDMA_PROTOCOL to use when I am the sender - one entry per receiver*/
    int64_t *rdma_protocol;
    
    /*Below are data structures that collect statistics for non-blocking point-to-point communications*/
    double *avg_compute_time_sender; //Avg. time diff between MPI_Wait and MPI_Isend for a sender in a sender-receiver pair
    double *avg_compute_time_receiver; //Avg. time diff between MPI_Wait and MPI_Irecv for a receiver in a sender-receiver pair
    double *avg_compute_time_opposite_receiver; //Avg. time diff between MPI_Wait and MPI_Irecv for the OPPOSITE receiver in a sender-receiver pair
    double *effective_time_diff; //Avg. time diff between arrival of an RNZ_START and posting of MPI_Irecv for a sender-receiver pair with sign
    double *curr_compute_time_sender; //Time diff between MPI_Wait and MPI_Isend for a sender in the current message
    double *curr_compute_time_receiver; //Time diff between MPI_Wait and MPI_Irecv for a receiver in the current message
    double *curr_effective_time_diff; //Time diff between arrival of an RNZ_START and posting of MPI_Irecv

    int64_t *number_of_messages_sent; //Number of messages sent by sender in a sender-receiver pair
    int64_t *number_of_messages_received; //Number of messages received by receiver in a sender-receiver pair
    double *avg_data_message_size_sent; //Avg. message size sent for sender in a sender-receiver pair
    double *avg_data_message_size_received; //Avg. message size received for a receiver in sender-receiver pair

    bool noUnsatDep(int iter, int tInd);	// there is no unsatisfied dependency for task
    void mark_all_done(int iter, int tInd);
    double taskExecTime(int tInd);
    void printStat();
    void check();
    void printState();

    void invertMsgPe(int iter, int tInd);
    double getTaskExecTime(int tInd);
    void addTaskExecTime(int tInd, double time);
    std::map<int, int>* msgDestLogs;
    int findTaskFromMsg(MsgID* msg);
    int numWth, numEmPes;

    KeyType pendingMsgs;
    KeyType pendingRMsgs;
    int64_t *sendSeq, *recvSeq;
    std::map<int, int> pendingReqs;
    std::map<int, int64_t> pendingRReqs;
    std::map<int64_t, int> reqIdToReceiverMapping; //Gives me the receiver PE id for a given non-blocking request id. Used in RDMA protocol tuning
    std::map<int64_t, int> reqIdToSenderMapping; //Gives me the receiver PE id for a given non-blocking request id. Used in RDMA protocol tuning
    std::map<int, std::list< MsgKey > > pendingReceivedPostMsgs; //List of arrived RECV_POST post messages corresponding to a request id. 

    std::map<MsgKey, int> receiveStatus; //Whether or not the MPI_Recv/MPI_Irecv for a given message has been posted
    std::list<MsgKey > pendingRnzStartMsgList; //List of arrived RNZ_START messages. Not all of them would be ready to respond to though

    //handling collectives
    std::vector<int64_t> collectiveSeq;
    std::map<int64_t, std::map<int64_t, std::map<int, int> > > pendingCollMsgs;
    CollKeyType pendingRCollMsgs;
    int64_t currentCollComm, currentCollSeq, currentCollTask, currentCollMsgSize;
    int currentCollRank, currentCollPartner, currentCollSize;
    int currentCollSendCount, currentCollRecvCount;
};

#endif /* PE_H_ */
