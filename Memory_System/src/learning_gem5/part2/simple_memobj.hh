/*
 * Copyright (c) 2017 Jason Lowe-Power
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LEARNING_GEM5_PART2_SIMPLE_MEMOBJ_HH__
#define __LEARNING_GEM5_PART2_SIMPLE_MEMOBJ_HH__

#include "mem/port.hh"
#include "params/SimpleMemobj.hh"
#include "sim/sim_object.hh"

namespace gem5
{

/**
 * A very simple memory object. Current implementation doesn't even cache
 * anything it just forwards requests and responses.
 * This memobj is fully blocking (not non-blocking). Only a single request can
 * be outstanding at a time.
 */
class SimpleMemobj : public SimObject
{
  private:

    class CPUSidePort : public ResponsePort
    {
      private:
        SimpleMemobj *owner;

        bool needRetry;

        PacketPtr blockedPacket;

      public:
        CPUSidePort(const std::string& name, SimpleMemobj *owner) :
            ResponsePort(name, owner), owner(owner), needRetry(false),
            blockedPacket(nullptr)
        { }

        void sendPacket(PacketPtr pkt);

        AddrRangeList getAddrRanges() const override;

        void trySendRetry();

      protected:

        Tick recvAtomic(PacketPtr pkt) override
        { panic("recvAtomic unimpl."); }

        void recvFunctional(PacketPtr pkt) override;

        bool recvTimingReq(PacketPtr pkt) override;

        void recvRespRetry() override;
    };

    class MemSidePort : public RequestPort
    {
      private:
        SimpleMemobj *owner;

        PacketPtr blockedPacket;

      public:
        MemSidePort(const std::string& name, SimpleMemobj *owner) :
            RequestPort(name, owner), owner(owner), blockedPacket(nullptr)
        { }

        void sendPacket(PacketPtr pkt);

      protected:

        bool recvTimingResp(PacketPtr pkt) override;

        void recvReqRetry() override;

        void recvRangeChange() override;
    };

    bool handleRequest(PacketPtr pkt);

    bool handleResponse(PacketPtr pkt);

    void handleFunctional(PacketPtr pkt);

    AddrRangeList getAddrRanges() const;

    void sendRangeChange();

    CPUSidePort instPort;
    CPUSidePort dataPort;

    MemSidePort memPort;

    bool blocked;

  public:

    SimpleMemobj(const SimpleMemobjParams &params);

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
};

} // namespace gem5

#endif // __LEARNING_GEM5_PART2_SIMPLE_MEMOBJ_HH__
