/*
 * Copyright (c) 2025 Regents of the University of California
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
#ifndef __NEW_META_HH__
#define __NEW_META_HH__

namespace gem5 {
// 1 GB is reserved for the permission table
#define ONE_G 0x40000000
// Cache lines are always 64 bytes
#define CACHE_LINE 0x40
// Then there are ppn mask  (4096)
#define PPN_MASK 0x1000
// FIXME:
// We need a template for a queue and a set. There can be multiple entries in
// the template for a given address to figure out where is the ID.
template<typename T>
class CustomQueue
{
    private:
        // There needs to be a queue
        std::queue<T> q;
        // There needs to be set or a map.
        std::unordered_set<T> s;

        // another map to keep track of binary search packts. When permission
        // number of packets are sent for that address, we remove this entry
        // from the f map. 
        std::unordered_map<uint64_t, unsigned int> f;
        std::unordered_map<uint64_t, bool> is_sent;
        // for popping entries make sure that a flag is set/unset
        // bool flag;
    public:
        void push(T val) {
            // Need to create two entries: one in the queue and the other in
            // the set.
            // regardless of the set, we'll always queue the queue.
            if (s.find(val) == s.end()) {
                q.push(val);
                s.insert(val);
            }
            if (auto search = f.find(val); search != f.end()) {
                search->second++;
            }
            else {
                // increment the count of seeing this packet by 1
                f.insert({val, 1});
            }
        }
        void set(T val) {
            is_sent[val] = true;
        }

        bool is_set(T val) {
            if (is_sent.find(val) != is_sent.end()) {
                return is_sent[val];
            }
            return false;
        }

        void unset(T val) {
            is_sent[val] = false;
        }
        void explicit_pop(T val) {
            // The response is made back in gem5.
            // we need to make sure that the entry is in the front of the queue
            if (q.front() != val) {
                std::cout << "Expected " << q.front() << " got " << val
                            << std::endl;
                assert(false && "This packet is not in the front of the queue\n");
            }
            // if it is in the front of the queue, we can pop it.
            // The packet can be removed from the tracker.
            s.erase(val);
            f.erase(val);
            q.pop();
        }
        T pop() {
            // The response is made back in gem5.
            T val = q.front();
            // pop the queue.
            // if (flag) {
            //     q.pop();
            //     // remove the entry from the set.
            //     // FIXME: The same port might get queued more than once.
                
            //     s.erase(val);
            // }
            // so, the logic is until N number of retries required for the 
            // permission packets are sent, we do not remove this retry request
            
            if (is_sent[val] == true){
                s.erase(val);
                f.erase(val);
                q.pop();
                // make this val ready to be sent again.
                unset(val);
            }

            return val;
        }

        T front() {
            // get the front of the queue.
            return q.front();
        }

        bool empty() {
            // is the queue empty>
            return q.empty();
        }
        size_t size() {
            // return the size of the queue
            return q.size();
        }
};

template<typename T>
class SimpleQueue
{
    private:
        // There needs to be a queue
        std::queue<T> q;

        // another map to keep track of binary search packts
        // std::unordered_set<T, Addr> f;
        // for popping entries make sure that a flag is set/unset
        bool flag;
    public:
        void push(T val) {
            // Need to create two entries: one in the queue and the other in
            // the set.
            q.push(val);
        }

        T pop() {
            // The response is made back in gem5.
            T val = q.front();
            // pop the queue.
            // if (flag) {
            q.pop();
            //     // remove the entry from the set.
            //     // FIXME: The same port might get queued more than once.
                
            //     s.erase(val);
            // }
            return val;
        }

        T front() {
            // get the front of the queue.
            return q.front();
        }

        bool empty() {
            // is the queue empty>
            return q.empty();
        }
        size_t size() {
            // return the size of the queue
            return q.size();
        }
};

template<typename T>
class OriginalQueue
{
    private:
        // There needs to be a queue
        std::queue<T> q;

        // another map to keep track of binary search packts
        std::unordered_set<T> s;
    public:
        void push(T val) {
            // Need to create two entries: one in the queue and the other in
            // the set.
            // regardless of the set, we'll always queue the queue.
            if (s.find(val) == s.end()) {
                q.push(val);
                s.insert(val);
            }
        }

        T pop() {
            // The response is made back in gem5.
            T val = q.front();
            q.pop();
            s.erase(val);
            return val;
        }

        T front() {
            // get the front of the queue.
            return q.front();
        }

        bool empty() {
            // is the queue empty>
            return q.empty();
        }
        size_t size() {
            // return the size of the queue
            return q.size();
        }
};


// make sure to keep a ENUM for the right model
enum model {
    MONDRIAN,
    FLAT_TABLE,
    DEACT,
    SPACE_CONTROL
};


// a class to keep track of the original packet's address
class PermissionSenderState : public Packet::SenderState {
public:
    // PacketPtr originalPkt;
    Addr originalAddr;

    PermissionSenderState(PacketPtr pkt)
        : originalAddr(pkt->getAddr()) {}
};

}


#endif