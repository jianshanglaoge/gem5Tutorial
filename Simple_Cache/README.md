# Creating a simple cache object
In this chapter, we will take the framework for a memory object we created in the last chapter and add caching logic to it.

## SimpleCache SimObject
After creating the SConscript file, that you can download here, we can create the SimObject Python file. We will call this simple memory object SimpleCache and create the SimObject Python file in src/learning_gem5/simple_cache.

```
from m5.params import *
from m5.proxy import *
from MemObject import MemObject

class SimpleCache(MemObject):
    type = 'SimpleCache'
    cxx_header = "learning_gem5/simple_cache/simple_cache.hh"

    cpu_side = VectorResponsePort("CPU side port, receives requests")
    mem_side = MasterPort("Memory side port, sends requests")

    latency = Param.Cycles(1, "Cycles taken on a hit or to resolve a miss")

    size = Param.MemorySize('16kB', "The size of the cache")

    system = Param.System(Parent.any, "The system this cache is part of")
```

There are a couple of differences between this SimObject file and the one from the previous chapter. First, we have a couple of extra parameters. Namely, a latency for cache accesses and the size of the cache. parameters-chapter goes into more detail about these kinds of SimObject parameters.

Next, we include a System parameter, which is a pointer to the main system this cache is connected to. This is needed so we can get the cache block size from the system object when we are initializing the cache. To reference the system object this cache is connected to, we use a special proxy parameter. In this case, we use Parent.any.

In the Python config file, when a SimpleCache is instantiated, this proxy parameter searches through all of the parents of the SimpleCache instance to find a SimObject that matches the System type. Since we often use a System as the root SimObject, you will often see a system parameter resolved with this proxy parameter.

The third and final difference between the SimpleCache and the SimpleMemobj is that instead of having two named CPU ports (inst_port and data_port), the SimpleCache use another special parameter: the VectorPort. VectorPorts behave similarly to regular ports (e.g., they are resolved via getPort), but they allow this object to connect to multiple peers. Then, in the resolution functions the parameter we ignored before (PortID idx) is used to differentiate between the different ports. By using a vector port, this cache can be connected into the system more flexibly than the SimpleMemobj.

## Implementing the SimpleCache
Most of the code for the SimpleCache is the same as the SimpleMemobj. There are a couple of changes in the constructor and the key memory object functions.

First, we need to create the CPU side ports dynamically in the constructor and initialize the extra member functions based on the SimObject parameters.

```
SimpleCache::SimpleCache(const SimpleCacheParams &params) :
    ClockedObject(params),
    latency(params.latency),
    blockSize(params.system->cacheLineSize()),
    capacity(params.size / blockSize),
    memPort(params.name + ".mem_side", this),
    blocked(false), originalPacket(nullptr), waitingPortId(-1), stats(this)
{
    for (int i = 0; i < params->port_cpu_side_connection_count; ++i) {
        cpuPorts.emplace_back(name() + csprintf(".cpu_side[%d]", i), i, this);
    }
}
```

In this function, we use the cacheLineSize from the system parameters to set the blockSize for this cache. We also initialize the capacity based on the block size and the parameter and initialize other member variables we will need below. Finally, we must create a number of CPUSidePorts based on the number of connections to this object. Since the cpu_side port was declared as a VectorResponsePort in the SimObject Python file, the parameter automatically has a variable port_cpu_side_connection_count. This is based on the Python name of the parameter. For each of these connections we add a new CPUSidePort to a cpuPorts vector declared in the SimpleCache class.

We also add one extra member variable to the CPUSidePort to save its id, and we add this as a parameter to its constructor.

Next, we need to implement getPort. The getPort is exactly the same as the SimpleMemobj. For getSlavePort, we now need to return the port based on the id requested.

```
Port &
SimpleCache::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "mem_side") {
        panic_if(idx != InvalidPortID,
                 "Mem side of simple cache not a vector port");
        return memPort;
    } else if (if_name == "cpu_side" && idx < cpuPorts.size()) {
        return cpuPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}
```
The implementation of the CPUSidePort and the MemSidePort is almost the same as in the SimpleMemobj. The only difference is we need to add an extra parameter to handleRequest that is the id of the port which the request originated. Without this id, we would not be able to forward the response to the correct port. The SimpleMemobj knew which port to send replies based on whether the original request was an instruction or data accesses. However, this information is not useful to the SimpleCache since it uses a vector of ports and not named ports.

The new handleRequest function does two different things than the handleRequest function in the SimpleMemobj. First, it stores the port id of the request as discussed above. Since the SimpleCache is blocking and only allows a single request outstanding at a time, we only need to save a single port id.

Second, it takes time to access a cache. Therefore, we need to take into account the latency to access the cache tags and the cache data for a request. We added an extra parameter to the cache object for this, and in handleRequest we now use an event to stall the request for the needed amount of time. We schedule a new event for latency cycles in the future. The clockEdge function returns the tick that the nth cycle in the future occurs on.

```
bool
SimpleCache::handleRequest(PacketPtr pkt, int port_id)
{
    if (blocked) {
        return false;
    }
    DPRINTF(SimpleCache, "Got request for addr %#x\n", pkt->getAddr());

    blocked = true;
	assert(waitingPortId == -1);
    waitingPortId = port_id;

    schedule(new EventFunctionWrapper([this, pkt]{ accessTiming(pkt); },
                                      name() + ".accessEvent", true),
             clockEdge(latency));
    return true;
}
```

This function first functionally accesses the cache. This function accessFunctional (described below) performs the functional access of the cache and either reads or writes the cache on a hit or returns that the access was a miss.

If the access is a hit, we simply need to respond to the packet. To respond, you first must call the function makeResponse on the packet. This converts the packet from a request packet to a response packet. For instance, if the memory command in the packet was a ReadReq this gets converted into a ReadResp. Writes behave similarly. Then, we can send the response back to the CPU.

The sendResponse function does the same things as the handleResponse function in the SimpleMemobj except that it uses the waitingPortId to send the packet to the right port. In this function, we need to mark the SimpleCache unblocked before calling sendPacket in case the peer on the CPU side immediately calls sendTimingReq. Then, we try to send retries to the CPU side ports if the SimpleCache can now receive requests and the ports need to be sent retries.

```
void SimpleCache::sendResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleCache, "Sending resp for addr %#x\n", pkt->getAddr());
    int port = waitingPortId;

    blocked = false;
    waitingPortId = -1;

    cpuPorts[port].sendPacket(pkt);
    for (auto& port : cpuPorts) {
        port.trySendRetry();
    }
}
```

Back to the accessTiming function, we now need to handle the cache miss case. On a miss, we first have to check to see if the missing packet is to an entire cache block. If the packet is aligned and the size of the request is the size of a cache block, then we can simply forward the request to memory, just like in the SimpleMemobj.

However, if the packet is smaller than a cache block, then we need to create a new packet to read the entire cache block from memory. Here, whether the packet is a read or a write request, we send a read request to memory to load the data for the cache block into the cache. In the case of a write, it will occur in the cache after we have loaded the data from memory.

Then, we create a new packet, that is blockSize in size and we call the allocate function to allocate memory in the Packet object for the data that we will read from memory. Note: this memory is freed when we free the packet. We use the original request object in the packet so the memory-side objects know the original requestor and the original request type for statistics.

Finally, we save the original packet pointer (pkt) in a member variable outstandingPacket so we can recover it when the SimpleCache receives a response. Then, we send the new packet across the memory side port.

```
void
SimpleCache::accessTiming(PacketPtr pkt)
{
    bool hit = accessFunctional(pkt);
    if (hit) {
	    stats.hits++;
        DDUMP(SimpleCache, pkt->getConstPtr<uint8_t>(), pkt->getSize());
        pkt->makeResponse();
        sendResponse(pkt);
    } else {
        stats.misses++;
        missTime = curTick();
        Addr addr = pkt->getAddr();
        Addr block_addr = pkt->getBlockAddr(blockSize);
        unsigned size = pkt->getSize();
        if (addr == block_addr && size == blockSize) {
            DPRINTF(SimpleCache, "forwarding packet\n");
            memPort.sendPacket(pkt);
        } else {
            DPRINTF(SimpleCache, "Upgrading packet to block size\n");
            panic_if(addr - block_addr + size > blockSize,
                     "Cannot handle accesses that span multiple cache lines");

            assert(pkt->needsResponse());
            MemCmd cmd;
            if (pkt->isWrite() || pkt->isRead()) {
                cmd = MemCmd::ReadReq;
            } else {
                panic("Unknown packet type in upgrade size");
            }

            PacketPtr new_pkt = new Packet(pkt->req, cmd, blockSize);
            new_pkt->allocate();
            assert(new_pkt->getAddr() == new_pkt->getBlockAddr(blockSize));
            outstandingPacket = pkt;

            memPort.sendPacket(new_pkt);
        }
    }
}
```

On a response from memory, we know that this was caused by a cache miss. The first step is to insert the responding packet into the cache.

Then, either there is an outstandingPacket, in which case we need to forward that packet to the original requestor, or there is no outstandingPacket which means we should forward the pkt in the response to the original requestor.

If the packet we are receiving as a response was an upgrade packet because the original request was smaller than a cache line, then we need to copy the new data to the outstandingPacket packet or write to the cache on a write. Then, we need to delete the new packet that we made in the miss handling logic.

```
bool
SimpleCache::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleCache, "Got response for addr %#x\n", pkt->getAddr());
    insert(pkt);

    stats.missLatency.sample(curTick() - missTime);
    if (outstandingPacket != nullptr) {
        bool hit = accessFunctional(originalPacket);
        panic_if(!hit, "Should always hit after inserting");
        outstandingPacket->makeResponse();
        delete pkt;
        pkt = outstandingPacket;
        outstandingPacket = nullptr;
    } // else, pkt contains the data it needs

    sendResponse(pkt);

    return true;
}
```

## Functional cache logic
Now, we need to implement two more functions: accessFunctional and insert. These two functions make up the key components of the cache logic.

First, to functionally update the cache, we first need storage for the cache contents. The simplest possible cache storage is a map (hashtable) that maps from addresses to data. Thus, we will add the following member to the SimpleCache.

```
std::unordered_map<Addr, uint8_t*> cacheStore;
```

To access the cache, we first check to see if there is an entry in the map which matches the address in the packet. We use the getBlockAddr function of the Packet type to get the block-aligned address. Then, we simply search for that address in the map. If we do not find the address, then this function returns false, the data is not in the cache, and it is a miss.

Otherwise, if the packet is a write request, we need to update the data in the cache. To do this, we write the data from the packet to the cache. We use the writeDataToBlock function which writes the data in the packet to the write offset into a potentially larger block of data. This function takes the cache block offset and the block size (as a parameter) and writes the correct offset into the pointer passed as the first parameter.

If the packet is a read request, we need to update the packet’s data with the data from the cache. The setDataFromBlock function performs the same offset calculation as the writeDataToBlock function, but writes the packet with the data from the pointer in the first parameter.

```
bool
SimpleCache::accessFunctional(PacketPtr pkt)
{
    Addr block_addr = pkt->getBlockAddr(blockSize);
    auto it = cacheStore.find(block_addr);
    if (it != cacheStore.end()) {
        if (pkt->isWrite()) {
            pkt->writeDataToBlock(it->second, blockSize);
        } else if (pkt->isRead()) {
            pkt->setDataFromBlock(it->second, blockSize);
        } else {
            panic("Unknown packet type!");
        }
        return true;
    }
    return false;
}
```

Finally, we also need to implement the insert function. This function is called every time the memory side port responds to a request.

The first step is to check if the cache is currently full. If the cache has more entries (blocks) than the capacity of the cache as set by the SimObject parameter, then we need to evict something. The following code evicts a random entry by leveraging the hashtable implementation of the C++ unordered_map.

On an eviction, we need to write the data back to the backing memory in case it has been updated. For this, we create a new Request-Packet pair. The packet uses a new memory command: MemCmd::WritebackDirty. Then, we send the packet across the memory side port (memPort) and erase the entry in the cache storage map.

Then, after a block has potentially been evicted, we add the new address to the cache. For this we simply allocate space for the block and add an entry to the map. Finally, we write the data from the response packet in to the newly allocated block. This data is guaranteed to be the size of the cache block since we made sure to make a new packet in the cache miss logic if the packet was smaller than a cache block.

```
void
SimpleCache::insert(PacketPtr pkt)
{
    assert(pkt->getAddr() ==  pkt->getBlockAddr(blockSize));
    assert(cacheStore.find(pkt->getAddr()) == cacheStore.end());
    assert(pkt->isResponse());

    if (cacheStore.size() >= capacity) {
        int bucket, bucket_size;
        do {
            bucket = random_mt.random(0, (int)cacheStore.bucket_count() - 1);
        } while ( (bucket_size = cacheStore.bucket_size(bucket)) == 0 );
        auto block = std::next(cacheStore.begin(bucket),
                               random_mt.random(0, bucket_size - 1));

        DPRINTF(SimpleCache, "Removing addr %#x\n", block->first);
        RequestPtr req = std::make_shared<Request>(
            block->first, blockSize, 0, 0);

        PacketPtr new_pkt = new Packet(req, MemCmd::WritebackDirty, blockSize);
        new_pkt->dataDynamic(block->second);

        DPRINTF(SimpleCache, "Writing packet back %s\n", pkt->print());
        memPort.sendPacket(new_pkt);

        cacheStore.erase(block->first);
    }

    DPRINTF(SimpleCache, "Inserting %s\n", pkt->print());
    DDUMP(SimpleCache, pkt->getConstPtr<uint8_t>(), blockSize);

    uint8_t *data = new uint8_t[blockSize];

    cacheStore[pkt->getAddr()] = data;

    pkt->writeDataToBlock(data, blockSize);
}

```

## Creating a config file for the cache
The last step in our implementation is to create a new Python config script that uses our cache. We can use the outline from the last chapter as a starting point. The only difference is we may want to set the parameters of this cache (e.g., set the size of the cache to 1kB) and instead of using the named ports (data_port and inst_port), we just use the cpu_side port twice. Since cpu_side is a VectorPort, it will automatically create multiple port connections.

```
import m5
from m5.objects import *

...

system.cache = SimpleCache(size='1kB')

system.cpu.icache_port = system.cache.cpu_side
system.cpu.dcache_port = system.cache.cpu_side

system.membus = SystemXBar()

system.cache.mem_side = system.membus.cpu_side_ports
system.workload = SEWorkload.init_compatible(binpath)
...
```

The Python config file can be downloaded here.

Running this script should produce the expected output from the hello binary.

```
gem5 Simulator System.  http://gem5.org
gem5 is copyrighted software; use the --copyright option for details.

gem5 compiled Jan 10 2017 17:38:15
gem5 started Jan 10 2017 17:40:03
gem5 executing on chinook, pid 29031
command line: build/X86/gem5.opt configs/learning_gem5/part2/simple_cache.py

Global frequency set at 1000000000000 ticks per second
warn: DRAM device capacity (8192 Mbytes) does not match the address range assigned (512 Mbytes)
0: system.remote_gdb.listener: listening for remote gdb #0 on port 7000
warn: CoherentXBar system.membus has no snooping ports attached!
warn: ClockedObject: More than one power state change request encountered within the same simulation tick
Beginning simulation!
info: Entering event queue @ 0.  Starting simulation...
Hello world!
Exiting @ tick 56082000 because target called exit()
```

Modifying the size of the cache, for instance to 128 KB, should improve the performance of the system.

```
gem5 Simulator System.  http://gem5.org
gem5 is copyrighted software; use the --copyright option for details.

gem5 compiled Jan 10 2017 17:38:15
gem5 started Jan 10 2017 17:41:10
gem5 executing on chinook, pid 29037
command line: build/X86/gem5.opt configs/learning_gem5/part2/simple_cache.py

Global frequency set at 1000000000000 ticks per second
warn: DRAM device capacity (8192 Mbytes) does not match the address range assigned (512 Mbytes)
0: system.remote_gdb.listener: listening for remote gdb #0 on port 7000
warn: CoherentXBar system.membus has no snooping ports attached!
warn: ClockedObject: More than one power state change request encountered within the same simulation tick
Beginning simulation!
info: Entering event queue @ 0.  Starting simulation...
Hello world!
Exiting @ tick 32685000 because target called exit()
```

## Adding statistics to the cache
Knowing the overall execution time of the system is one important metric. However, you may want to include other statistics as well, such as the hit and miss rates of the cache. To do this, we need to add some statistics to the SimpleCache object.

First, we need to declare the statistics in the SimpleCache object. They are part of the Stats namespace. In this case, we’ll make four statistics. The number of hits and the number of misses are just simple Scalar counts. We will also add a missLatency which is a histogram of the time it takes to satisfy a miss. Finally, we’ll add a special statistic called a Formula for the hitRatio that is a combination of other statistics (the number of hits and misses).

```
class SimpleCache : public MemObject
{
  protect:
    ...
    struct SimpleCacheStats : public statistics::Group
    {
        SimpleCacheStats(statistics::Group *parent);
        statistics::Scalar hits;
        statistics::Scalar misses;
        statistics::Histogram missLatency;
        statistics::Formula hitRatio;
    } stats;

};
```

Next, we have to define the function to override the regStats function so the statistics are registered with gem5’s statistics infrastructure. Here, for each statistic, we give it a name based on the “parent” SimObject name and a description. For the histogram statistic, we also need to initialize it with how many buckets we want in the histogram. Finally, for the formula, we simply need to write the formula down in code.

```
SimpleCache::SimpleCacheStats::SimpleCacheStats(statistics::Group *parent)
      : statistics::Group(parent),
      ADD_STAT(hits, statistics::units::Count::get(), "Number of hits"),
      ADD_STAT(misses, statistics::units::Count::get(), "Number of misses"),
      ADD_STAT(missLatency, statistics::units::Tick::get(),
               "Ticks for misses to the cache"),
      ADD_STAT(hitRatio, statistics::units::Ratio::get(),
               "The ratio of hits to the total accesses to the cache",
               hits / (hits + misses))
{
    missLatency.init(16); // number of buckets
}

```

Finally, we need to use update the statistics in our code. In the accessTiming class, we can increment the hits and misses on a hit and miss respectively. Additionally, on a miss, we save the current time so we can measure the latency.

```
void
SimpleCache::accessTiming(PacketPtr pkt)
{
    bool hit = accessFunctional(pkt);
    if (hit) {
        // Respond to the CPU side
        stats.hits++; // update stats
        DDUMP(SimpleCache, pkt->getConstPtr<uint8_t>(), pkt->getSize());
        pkt->makeResponse();
        sendResponse(pkt);
    } else {
        stats.misses++; // update stats
        missTime = curTick();
        ...
```

Then, when we get a response, we need to add the measured latency to our histogram. For this, we use the sample function. This adds a single point to the histogram. This histogram automatically resizes the buckets to fit the data it receives.

```
bool
SimpleCache::handleResponse(PacketPtr pkt)
{
    insert(pkt);

    stats.missLatency.sample(curTick() - missTime);
    ...
```

The complete code for the SimpleCache header file can be downloaded here, and the complete code for the implementation of the SimpleCache can be downloaded here.

Now, if we run the above config file, we can check on the statistics in the stats.txt file. For the 1 KB case, we get the following statistics. 91% of the accesses are hits and the average miss latency is 53334 ticks (or 53 ns).

```
system.cache.hits                                9225                       # Number of hits (Count)
system.cache.misses                               879                       # Number of misses (Count)
system.cache.missLatency::samples                 879                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::mean           49341.296928                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::gmean          42148.038159                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::stdev          33239.957480                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::0-32767                 296     33.67%     33.67% # Ticks for misses to the cache (Tick)
system.cache.missLatency::32768-65535             439     49.94%     83.62% # Ticks for misses to the cache (Tick)
system.cache.missLatency::65536-98303             106     12.06%     95.68% # Ticks for misses to the cache (Tick)
system.cache.missLatency::98304-131071             15      1.71%     97.38% # Ticks for misses to the cache (Tick)
system.cache.missLatency::131072-163839            14      1.59%     98.98% # Ticks for misses to the cache (Tick)
system.cache.missLatency::163840-196607             3      0.34%     99.32% # Ticks for misses to the cache (Tick)
system.cache.missLatency::196608-229375             0      0.00%     99.32% # Ticks for misses to the cache (Tick)
system.cache.missLatency::229376-262143             1      0.11%     99.43% # Ticks for misses to the cache (Tick)
system.cache.missLatency::262144-294911             0      0.00%     99.43% # Ticks for misses to the cache (Tick)
system.cache.missLatency::294912-327679             4      0.46%     99.89% # Ticks for misses to the cache (Tick)
system.cache.missLatency::327680-360447             1      0.11%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::360448-393215             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::393216-425983             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::425984-458751             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::458752-491519             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::491520-524287             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::total                   879                       # Ticks for misses to the cache (Tick)
system.cache.hitRatio                        0.913005                       # The ratio of hits to the total accesses to the cache (Ratio)
```

And when using a 128 KB cache, we get a slightly higher hit ratio. It seems like our cache is working as expected!

```
system.cache.hits                                9739                       # Number of hits (Count)
system.cache.misses                               365                       # Number of misses (Count)
system.cache.missLatency::samples                 365                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::mean           57449.315068                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::gmean          55350.781964                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::stdev          24177.702488                       # Ticks for misses to the cache (Tick)
system.cache.missLatency::0-32767                   0      0.00%      0.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::32768-65535             298     81.64%     81.64% # Ticks for misses to the cache (Tick)
system.cache.missLatency::65536-98303              64     17.53%     99.18% # Ticks for misses to the cache (Tick)
system.cache.missLatency::98304-131071              0      0.00%     99.18% # Ticks for misses to the cache (Tick)
system.cache.missLatency::131072-163839             0      0.00%     99.18% # Ticks for misses to the cache (Tick)
system.cache.missLatency::163840-196607             0      0.00%     99.18% # Ticks for misses to the cache (Tick)
system.cache.missLatency::196608-229375             1      0.27%     99.45% # Ticks for misses to the cache (Tick)
system.cache.missLatency::229376-262143             0      0.00%     99.45% # Ticks for misses to the cache (Tick)
system.cache.missLatency::262144-294911             0      0.00%     99.45% # Ticks for misses to the cache (Tick)
system.cache.missLatency::294912-327679             1      0.27%     99.73% # Ticks for misses to the cache (Tick)
system.cache.missLatency::327680-360447             1      0.27%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::360448-393215             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::393216-425983             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::425984-458751             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::458752-491519             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::491520-524287             0      0.00%    100.00% # Ticks for misses to the cache (Tick)
system.cache.missLatency::total                   365                       # Ticks for misses to the cache (Tick)
system.cache.hitRatio                        0.963876                       # The ratio of hits to the total accesses to the cache (Ratio)
```

## Extend Lab: Finding the relationship between size of the cache and hitRatio

In this lab, we want try to find the relationship between cache size and hit ratio. We reset the cache size in the script

```
system.cache = SimpleCache(size='1kB')
```

The value of cache will be changed from 1kB, 2kB, 4kB, 8kB, 16kB, 32kB, 64kB, 128kB. And we create a histogram to show if this relationship is linear?

