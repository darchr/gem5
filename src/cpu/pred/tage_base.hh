/*
 * Copyright (c) 2014 The University of Wisconsin
 *
 * Copyright (c) 2006 INRIA (Institut National de Recherche en
 * Informatique et en Automatique  / French National Research Institute
 * for Computer Science and Applied Mathematics)
 *
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

/* @file
 * Implementation of a TAGE branch predictor. TAGE is a global-history based
 * branch predictor. It features a PC-indexed bimodal predictor and N
 * partially tagged tables, indexed with a hash of the PC and the global
 * branch history. The different lengths of global branch history used to
 * index the partially tagged tables grow geometrically. A small path history
 * is also used in the hash.
 *
 * All TAGE tables are accessed in parallel, and the one using the longest
 * history that matches provides the prediction (some exceptions apply).
 * Entries are allocated in components using a longer history than the
 * one that predicted when the prediction is incorrect.
 */

#ifndef __CPU_PRED_TAGE_BASE
#define __CPU_PRED_TAGE_BASE

#include <vector>

#include "base/statistics.hh"
#include "cpu/null_static_inst.hh"
#include "cpu/static_inst.hh"
#include "params/TAGEBase.hh"
#include "sim/sim_object.hh"

class TAGEBase : public SimObject
{
  public:
    TAGEBase(const TAGEBaseParams &p);
    void init() override;

  protected:
    // Prediction Structures

    // Tage Entry
    struct TageEntry
    {
        int8_t ctr;
        uint16_t tag;
        uint8_t u;
        TageEntry() : ctr(0), tag(0), u(0) { }
    };

    // Folded History Table - compressed history
    // to mix with instruction PC to index partially
    // tagged tables.
    struct FoldedHistory
    {
        unsigned comp;
        int compLength;
        int origLength;
        int outpoint;
        int bufferSize;

        FoldedHistory()
        {
            comp = 0;
        }

        void init(int original_length, int compressed_length)
        {
            origLength = original_length;
            compLength = compressed_length;
            outpoint = original_length % compressed_length;
        }

        void update(uint8_t * h)
        {
            comp = (comp << 1) | h[0];
            comp ^= h[origLength] << outpoint;
            comp ^= (comp >> compLength);
            comp &= (1ULL << compLength) - 1;
        }
    };

  public:

    // provider type
    enum
    {
        BIMODAL_ONLY = 0,
        TAGE_LONGEST_MATCH,
        BIMODAL_ALT_MATCH,
        TAGE_ALT_MATCH,
        LAST_TAGE_PROVIDER_TYPE = TAGE_ALT_MATCH
    };

    // Primary branch history entry
    struct BranchInfo
    {
        int pathHist;
        int ptGhist;
        int hitBank;
        int hitBankIndex;
        int altBank;
        int altBankIndex;
        int bimodalIndex;

        bool tagePred;
        bool altTaken;
        bool condBranch;
        bool longestMatchPred;
        bool pseudoNewAlloc;
        Addr branchPC;

        // Pointer to dynamically allocated storage
        // to save table indices and folded histories.
        // To do one call to new instead of five.
        int *storage;

        // Pointers to actual saved array within the dynamically
        // allocated storage.
        int *tableIndices;
        int *tableTags;
        int *ci;
        int *ct0;
        int *ct1;

        // for stats purposes
        unsigned provider;

        BranchInfo(const TAGEBase &tage)
            : pathHist(0), ptGhist(0),
              hitBank(0), hitBankIndex(0),
              altBank(0), altBankIndex(0),
              bimodalIndex(0),
              tagePred(false), altTaken(false),
              condBranch(false), longestMatchPred(false),
              pseudoNewAlloc(false), branchPC(0),
              provider(-1)
        {
            int sz = tage.nHistoryTables + 1;
            storage = new int [sz * 5];
            tableIndices = storage;
            tableTags = storage + sz;
            ci = tableTags + sz;
            ct0 = ci + sz;
            ct1 = ct0 + sz;
        }

        virtual ~BranchInfo()
        {
            delete[] storage;
        }
    };

    virtual BranchInfo *makeBranchInfo();

    /**
     * Computes the index used to access the
     * bimodal table.
     * @param pc_in The unshifted branch PC.
     */
    virtual int bindex(Addr pc_in) const;

    /**
     * Computes the index used to access a
     * partially tagged table.
     * @param tid The thread ID used to select the
     * global histories to use.
     * @param pc The unshifted branch PC.
     * @param bank The partially tagged table to access.
     */
    virtual int gindex(ThreadID tid, Addr pc, int bank) const;

    /**
     * Utility function to shuffle the path history
     * depending on which tagged table we are accessing.
     * @param phist The path history.
     * @param size Number of path history bits to use.
     * @param bank The partially tagged table to access.
     */
    virtual int F(int phist, int size, int bank) const;

    /**
     * Computes the partial tag of a tagged table.
     * @param tid the thread ID used to select the
     * global histories to use.
     * @param pc The unshifted branch PC.
     * @param bank The partially tagged table to access.
     */
    virtual uint16_t gtag(ThreadID tid, Addr pc, int bank) const;

    /**
     * Updates a direction counter based on the actual
     * branch outcome.
     * @param ctr Reference to counter to update.
     * @param taken Actual branch outcome.
     * @param nbits Counter width.
     */
    template<typename T>
    static void ctrUpdate(T & ctr, bool taken, int nbits);

    /**
     * Updates an unsigned counter based on up/down parameter
     * @param ctr Reference to counter to update.
     * @param up Boolean indicating if the counter is incremented/decremented
     * If true it is incremented, if false it is decremented
     * @param nbits Counter width.
     */
    static void unsignedCtrUpdate(uint8_t & ctr, bool up, unsigned nbits);

    /**
     * Get a branch prediction from the bimodal
     * predictor.
     * @param pc The unshifted branch PC.
     * @param bi Pointer to information on the
     * prediction.
     */
    virtual bool getBimodePred(Addr pc, BranchInfo* bi) const;

    /**
     * Updates the bimodal predictor.
     * @param pc The unshifted branch PC.
     * @param taken The actual branch outcome.
     * @param bi Pointer to information on the prediction
     * recorded at prediction time.
     */
    void baseUpdate(Addr pc, bool taken, BranchInfo* bi);

   /**
    * (Speculatively) updates the global branch history.
    * @param h Reference to pointer to global branch history.
    * @param dir (Predicted) outcome to update the histories
    * with.
    * @param tab
    * @param PT Reference to path history.
    */
    void updateGHist(uint8_t * &h, bool dir, uint8_t * tab, int &PT);

    /**
     * Update TAGE. Called at execute to repair histories on a misprediction
     * and at commit to update the tables.
     * @param tid The thread ID to select the global
     * histories to use.
     * @param branch_pc The unshifted branch PC.
     * @param taken Actual branch outcome.
     * @param bi Pointer to information on the prediction
     * recorded at prediction time.
     */
    void update(ThreadID tid, Addr branch_pc, bool taken, BranchInfo* bi);

   /**
    * (Speculatively) updates global histories (path and direction).
    * Also recomputes compressed (folded) histories based on the
    * branch direction.
    * @param tid The thread ID to select the histories
    * to update.
    * @param branch_pc The unshifted branch PC.
    * @param taken (Predicted) branch direction.
    * @param b Wrapping pointer to BranchInfo (to allow
    * storing derived class prediction information in the
    * base class).
    */
    virtual void updateHistories(
        ThreadID tid, Addr branch_pc, bool taken, BranchInfo* b,
        bool speculative,
        const StaticInstPtr & inst = nullStaticInstPtr,
        Addr target = MaxAddr);

    /**
     * Restores speculatively updated path and direction histories.
     * Also recomputes compressed (folded) histories based on the
     * correct branch outcome.
     * This version of squash() is called once on a branch misprediction.
     * @param tid The Thread ID to select the histories to rollback.
     * @param taken The correct branch outcome.
     * @param bp_history Wrapping pointer to BranchInfo (to allow
     * storing derived class prediction information in the
     * base class).
     * @param target The correct branch target
     * @post bp_history points to valid memory.
     */
    virtual void squash(
        ThreadID tid, bool taken, BranchInfo *bi, Addr target);

    /**
     * Update TAGE for conditional branches.
     * @param branch_pc The unshifted branch PC.
     * @param taken Actual branch outcome.
     * @param bi Pointer to information on the prediction
     * recorded at prediction time.
     * @nrand Random int number from 0 to 3
     * @param corrTarget The correct branch target
     * @param pred Final prediction for this branch
     * @param preAdjustAlloc call adjustAlloc before checking
     * pseudo newly allocated entries
     */
    virtual void condBranchUpdate(
        ThreadID tid, Addr branch_pc, bool taken, BranchInfo* bi,
        int nrand, Addr corrTarget, bool pred, bool preAdjustAlloc = false);

    /**
     * TAGE prediction called from TAGE::predict
     * @param tid The thread ID to select the global
     * histories to use.
     * @param branch_pc The unshifted branch PC.
     * @param cond_branch True if the branch is conditional.
     * @param bi Pointer to the BranchInfo
     */
    bool tagePredict(
        ThreadID tid, Addr branch_pc, bool cond_branch, BranchInfo* bi);

    /**
     * Update the stats
     * @param taken Actual branch outcome
     * @param bi Pointer to information on the prediction
     * recorded at prediction time.
     */
    virtual void updateStats(bool taken, BranchInfo* bi);

    /**
     * Instantiates the TAGE table entries
     */
    virtual void buildTageTables();

    /**
     * Calculates the history lengths
     * and some other paramters in derived classes
     */
    virtual void calculateParameters();

    /**
     * On a prediction, calculates the TAGE indices and tags for
     * all the different history lengths
     */
    virtual void calculateIndicesAndTags(
        ThreadID tid, Addr branch_pc, BranchInfo* bi);

    /**
     * Calculation of the index for useAltPredForNewlyAllocated
     * On this base TAGE implementation it is always 0
     */
    virtual unsigned getUseAltIdx(BranchInfo* bi, Addr branch_pc);

    /**
     * Extra calculation to tell whether TAGE allocaitons may happen or not
     * on an update
     * For this base TAGE implementation it does nothing
     */
    virtual void adjustAlloc(bool & alloc, bool taken, bool pred_taken);

    /**
     * Handles Allocation and U bits reset on an update
     */
    virtual void handleAllocAndUReset(
        bool alloc, bool taken, BranchInfo* bi, int nrand);

    /**
     * Handles the U bits reset
     */
    virtual void handleUReset();

    /**
     * Handles the update of the TAGE entries
     */
    virtual void handleTAGEUpdate(
        Addr branch_pc, bool taken, BranchInfo* bi);

    /**
     * Algorithm for resetting a single U counter
     */
    virtual void resetUctr(uint8_t & u);

    /**
     * Extra steps for calculating altTaken
     * For this base TAGE class it does nothing
     */
    virtual void extraAltCalc(BranchInfo* bi);

    virtual bool isHighConfidence(BranchInfo* bi) const
    {
        return false;
    }

    void btbUpdate(ThreadID tid, Addr branch_addr, BranchInfo* &bi);
    unsigned getGHR(ThreadID tid, BranchInfo *bi) const;
    int8_t getCtr(int hitBank, int hitBankIndex) const;
    unsigned getTageCtrBits() const;
    int getPathHist(ThreadID tid) const;
    bool isSpeculativeUpdateEnabled() const;
    size_t getSizeInBits() const;

  protected:
    const unsigned logRatioBiModalHystEntries;
    const unsigned nHistoryTables;
    const unsigned tagTableCounterBits;
    const unsigned tagTableUBits;
    const unsigned histBufferSize;
    const unsigned minHist;
    const unsigned maxHist;
    const unsigned pathHistBits;

    std::vector<unsigned> tagTableTagWidths;
    std::vector<int> logTagTableSizes;

    std::vector<bool> btablePrediction;
    std::vector<bool> btableHysteresis;
    TageEntry **gtable;

    // Keep per-thread histories to
    // support SMT.
    struct ThreadHistory
    {
        // Speculative path history
        // (LSB of branch address)
        int pathHist;

        // Speculative branch direction
        // history (circular buffer)
        // @TODO Convert to std::vector<bool>
        uint8_t *globalHistory;

        // Pointer to most recent branch outcome
        uint8_t* gHist;

        // Index to most recent branch outcome
        int ptGhist;

        // Speculative folded histories.
        FoldedHistory *computeIndices;
        FoldedHistory *computeTags[2];
    };

    std::vector<ThreadHistory> threadHistory;

    /**
     * Initialization of the folded histories
     */
    virtual void initFoldedHistories(ThreadHistory & history);

    int *histLengths;
    int *tableIndices;
    int *tableTags;

    std::vector<int8_t> useAltPredForNewlyAllocated;
    int64_t tCounter;
    uint64_t logUResetPeriod;
    const int64_t initialTCounterValue;
    unsigned numUseAltOnNa;
    unsigned useAltOnNaBits;
    unsigned maxNumAlloc;

    // Tells which tables are active
    // (for the base TAGE implementation all are active)
    // Some other classes use this for handling associativity
    std::vector<bool> noSkip;

    const bool speculativeHistUpdate;

    const unsigned instShiftAmt;

    bool initialized;

    struct TAGEBaseStats : public Stats::Group
    {
        TAGEBaseStats(Stats::Group *parent, unsigned nHistoryTables);
        // stats
        Stats::Scalar longestMatchProviderCorrect;
        Stats::Scalar altMatchProviderCorrect;
        Stats::Scalar bimodalAltMatchProviderCorrect;
        Stats::Scalar bimodalProviderCorrect;
        Stats::Scalar longestMatchProviderWrong;
        Stats::Scalar altMatchProviderWrong;
        Stats::Scalar bimodalAltMatchProviderWrong;
        Stats::Scalar bimodalProviderWrong;
        Stats::Scalar altMatchProviderWouldHaveHit;
        Stats::Scalar longestMatchProviderWouldHaveHit;

        Stats::Vector longestMatchProvider;
        Stats::Vector altMatchProvider;
    } stats;
};

#endif // __CPU_PRED_TAGE_BASE
