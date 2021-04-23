/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief implementation for PBFTEngine
 * @file PBFTEngine.h
 * @author: yujiechen
 * @date 2021-04-20
 */
#pragma once
#include "core/ConsensusEngine.h"
#include <bcos-framework/libutilities/ConcurrentQueue.h>
#include <bcos-framework/libutilities/Error.h>


namespace bcos
{
namespace front
{
class FrontServiceInterface;
}
namespace consensus
{
class PBFTBaseMessageInterface;
class PBFTMessageInterface;
class ViewChangeMsgInterface;
class NewViewMsgInterface;
class PBFTMessageFactory;
class PBFTConfig;
class PBFTCodecInterface;
class ValidatorInterface;
class PBFTReqCache;
class PBFTProposalInterface;

using PBFTMsgQueue = ConcurrentQueue<std::shared_ptr<PBFTBaseMessageInterface>>;
using PBFTMsgQueuePtr = std::shared_ptr<PBFTMsgQueue>;

enum CheckResult
{
    VALID = 0,
    INVALID = 1,
    FUTURE = 2
};

class PBFTEngine : public ConsensusEngine, public std::enable_shared_from_this<PBFTEngine>
{
public:
    PBFTEngine(std::shared_ptr<PBFTConfig> _config, std::shared_ptr<PBFTCodecInterface> _codec,
        std::shared_ptr<PBFTMessageFactory> _pbftMessageFactory,
        std::shared_ptr<ValidatorInterface> _validator,
        std::shared_ptr<bcos::front::FrontServiceInterface> _frontService,
        bool _needVerifyProposal);

    ~PBFTEngine() override { stop(); }

    void start() override;
    void stop() override;

protected:
    // Receive PBFT message package from frontService
    virtual void onReceivePBFTMessage(bcos::Error::Ptr _error, bcos::crypto::NodeIDPtr _nodeID,
        bytesConstRef _data, std::function<void(bytesConstRef _respData)> _respFunc);

    // PBFT main processing function
    void executeWorker() override;

    // General entry for message processing
    virtual void handleMsg(std::shared_ptr<PBFTBaseMessageInterface> _msg);

    // Process Pre-prepare type message packets
    virtual bool handlePrePrepareMsg(
        std::shared_ptr<PBFTMessageInterface> _prePrepareMsg, bool _needVerifyProposal);
    virtual CheckResult checkPrePrepareMsg(std::shared_ptr<PBFTMessageInterface> _prePrepareMsg);
    virtual CheckResult checkSignature(std::shared_ptr<PBFTBaseMessageInterface> _req);
    virtual CheckResult checkPBFTMsgState(std::shared_ptr<PBFTBaseMessageInterface> _pbftReq) const;
    virtual void broadcastPrepareMsg(std::shared_ptr<PBFTMessageInterface> _prePrepareMsg);

    // Process the Prepare type message packet
    virtual bool handlePrepareMsg(std::shared_ptr<PBFTMessageInterface> _prepareMsg);
    virtual CheckResult checkPrepareMsg(std::shared_ptr<PBFTMessageInterface> _prepareMsg);
    virtual void tryToPrecommit(std::shared_ptr<PBFTMessageInterface> _prepareMsg);

    virtual bool handleCommitMsg(std::shared_ptr<PBFTMessageInterface> _commitMsg);
    virtual bool handleViewChangeMsg(std::shared_ptr<ViewChangeMsgInterface> _viewChangeMsg);
    virtual bool handleNewViewMsg(std::shared_ptr<NewViewMsgInterface> _newViewMsg);

private:
    // utility functions
    void waitSignal()
    {
        boost::unique_lock<boost::mutex> l(x_signalled);
        m_signalled.wait_for(l, boost::chrono::milliseconds(5));
    }
    std::string printCurrentState();

private:
    // PBFT configuration class
    // mainly maintains the node information, consensus configuration information
    // such as consensus node list, consensus weight, etc.
    std::shared_ptr<PBFTConfig> m_config;
    // Codec for serialization/deserialization of PBFT message packets
    std::shared_ptr<PBFTCodecInterface> m_codec;
    // Factory for creating PBFT message package
    std::shared_ptr<PBFTMessageFactory> m_pbftMessageFactory;
    // Proposal validator
    std::shared_ptr<ValidatorInterface> m_validator;
    // FrontService, used to send/receive P2P message packages
    std::shared_ptr<bcos::front::FrontServiceInterface> m_frontService;

    // PBFT message cache queue
    PBFTMsgQueuePtr m_msgQueue;
    std::shared_ptr<PBFTReqCache> m_pbftReqCache;
    // Flag, used to indicate whether Proposal needs to be verified
    bool m_needVerifyProposal;

    boost::condition_variable m_signalled;
    boost::mutex x_signalled;
    mutable Mutex m_mutex;

    const unsigned c_PopWaitSeconds = 5;
    const unsigned c_pbftMsgDefaultVersion = 0;
};
}  // namespace consensus
}  // namespace bcos