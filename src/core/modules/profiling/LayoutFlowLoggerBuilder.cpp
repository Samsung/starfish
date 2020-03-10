/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "LayoutFlowLoggerBuilder.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/layout/Frame.h"

namespace Starfish {

int LayoutFlowLoggerBuilder::INDENT_COUNTER = 0;

static void logLayoutInformation(UserData* data)
{
    if (data->m_frame) {
        printf("[%p] ", data->m_frame);
        if (data->m_frame->node()) {
            auto n = data->m_frame->node();
            printf("[%s] ", CSTR(n->localName()));
            if (n->isElement()) {
                printf("[%s][%s] ", CSTR(n->asElement()->id()),
                       CSTR(n->asElement()->className()));
            }
        }
    }
    if (data->infoMessage) {
        printf("%s ", data->infoMessage);
    }
    switch (data->resolveWhat) {
    case Frame::LayoutWantToResolve::ResolveWidth:
        printf("ResolveWidth");
        break;
    case Frame::LayoutWantToResolve::ResolveHeight:
        printf("ResolveHeight");
        break;
    case Frame::LayoutWantToResolve::ResolveAll:
        printf("ResolveAll");
    default:
        break;
    }
    printf("\n");
}

LayoutFlowLoggerBuilder::LayoutFlowLoggerBuilder(UserData& userData)
    : m_userData(userData)
{
}

LayoutFlowLoggerBuilder::~LayoutFlowLoggerBuilder()
{
}

void LayoutFlowLoggerBuilder::buildUserData()
{
    m_logger->setUserData(&m_userData);
}

void LayoutFlowLoggerBuilder::buildIndentCounter()
{
    m_logger->setIndentCounter(&INDENT_COUNTER);
}

void LayoutFlowLoggerBuilder::buildIncommingMessageWritter()
{
    m_logger->setIncommingMessageWritter([](void* data) {
        auto ud = (UserData*)data;
        if (ud->incommingMessage) {
            printf("%s", ud->incommingMessage);
        }
        logLayoutInformation(ud);
    });
}

void LayoutFlowLoggerBuilder::buildOutgoingMessageWritter()
{
    m_logger->setOutgoingMessageWritter([](void* data) {
        auto ud = (UserData*)data;
        if (ud->outgoingMessage) {
            printf("%s", ud->outgoingMessage);
        }
        logLayoutInformation(ud);
    });
}
}
