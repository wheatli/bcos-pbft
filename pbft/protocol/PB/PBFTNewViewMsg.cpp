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
 * @brief implementation for PBFTNewViewMsg
 * @file PBFTNewViewMsg.cpp
 * @author: yujiechen
 * @date 2021-04-16
 */

#include "PBFTNewViewMsg.h"
#include "pbft/protocol/PB/PBFTMessage.h"
#include "pbft/protocol/PB/PBFTViewChangeMsg.h"
#include <bcos-framework/libprotocol/Common.h>

using namespace bcos;
using namespace bcos::consensus;
using namespace bcos::protocol;
using namespace bcos::crypto;
bytesPointer PBFTNewViewMsg::encode(CryptoSuite::Ptr, KeyPairInterface::Ptr) const
{
    return encodePBObject(m_rawNewView);
}

void PBFTNewViewMsg::decode(bytesConstRef _data)
{
    decodePBObject(m_rawNewView, _data);
    setBaseMessage(std::shared_ptr<BaseMessage>(m_rawNewView->mutable_message()));
    PBFTNewViewMsg::deserializeToObject();
}

void PBFTNewViewMsg::deserializeToObject()
{
    PBFTBaseMessage::deserializeToObject();
    // decode into m_generatedPreprepare
    if (m_rawNewView->has_generatedpreprepare())
    {
        std::shared_ptr<PBFTRawMessage> pbRawPrePrepareObject(
            m_rawNewView->mutable_generatedpreprepare());
        m_generatedPreprepare = std::make_shared<PBFTMessage>(pbRawPrePrepareObject);
    }
    // decode into m_viewChangeList
    for (int i = 0; i < m_rawNewView->viewchangemsglist_size(); i++)
    {
        std::shared_ptr<RawViewChangeMessage> pbRawViewChange(
            m_rawNewView->mutable_viewchangemsglist(i));
        m_viewChangeList->push_back(std::make_shared<PBFTViewChangeMsg>(pbRawViewChange));
    }
}

void PBFTNewViewMsg::setViewChangeMsgList(ViewChangeMsgList const& _viewChangeMsgList)
{
    *m_viewChangeList = _viewChangeMsgList;
    for (auto viewChangeMsg : _viewChangeMsgList)
    {
        auto pbViewChangeMsg = std::dynamic_pointer_cast<PBFTViewChangeMsg>(viewChangeMsg);
        m_rawNewView->mutable_viewchangemsglist()->AddAllocated(
            pbViewChangeMsg->rawViewChange().get());
    }
}

void PBFTNewViewMsg::setGeneratedPrePrepare(PBFTBaseMessageInterface::Ptr _prePreparedMsg)
{
    m_generatedPreprepare = _prePreparedMsg;
    auto pbPreprepare = std::dynamic_pointer_cast<PBFTMessage>(_prePreparedMsg);
    m_rawNewView->set_allocated_generatedpreprepare(pbPreprepare->pbftRawMessage().get());
}