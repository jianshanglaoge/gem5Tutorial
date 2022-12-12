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

#include "learning_gem5/part2/clock_count_object.hh"

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/HelloExample.hh"
#include "sim/sim_exit.hh"

namespace gem5
{
ClockCountObject::ClockCountObject(const ClockCountObjectParams &params):
    SimObject(params), event([this]{ startsim(); }, name() + ".event"),
    firstnumber(params.firstnumber),clkfreq(params.clkfreq),runtime(params.runtime)
{
    counter.reset(new int (firstnumber));
    DPRINTF(HelloExample, "construct finished\n");
}

void 
ClockCountObject::startsim()
{
    int alltick = runtime * clkfreq;
    processEvent();
    std::cout<<"curtime: "<<curTick()<<std::endl;
    if(curTick() < alltick)
    {
        schedule(event, curTick() + 1);
    }
    std::cout<<counter[0]<<std::endl;
}

void 
ClockCountObject::startup()
{
    schedule(event, curTick());
}

void
ClockCountObject::processEvent()
{
    counter[0] = counter[0] + 1;
}

} // namespace gem5
