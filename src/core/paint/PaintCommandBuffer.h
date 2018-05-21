/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifdef STARFISH_ENABLE_CANVAS

#ifndef __StarFishPaintCommandBuffer__
#define __StarFishPaintCommandBuffer__

#include "StarFishConfig.h"
#include "StarFish.h"

#include "core/modules/canvas/Canvas.h"

namespace StarFish {

class PaintCommandArgument {
public:
    enum Type {
        // STR,
        NUM,
    };

    PaintCommandArgument()
    {
    }

    PaintCommandArgument(Type type, double num)
        : m_type(type)
        , m_num(num)
    {
    }

    double value()
    {
        return m_num;
    }

    Type m_type;
    union {
        // String* m_str;
        double m_num;
    };
};

class PaintCommand {
public:
    enum Command {
        // 2D
        SETCOLOR_2D,
        MOVETO_2D,
        LINETO_2D,
        STROKE_2D,
        FILLRECT_2D,
        SAVE_2D,
        RESTORE_2D,
        BEZIERCURVETO_2D,
        SETLINEWIDTH_2D,
        CLEARRECT_2D,
        // 3D for 'opengl' or 'direct X'
    };

    PaintCommand(Command type)
        : m_type(type)
    {
    }

    void insertArgumentNumber(double n)
    {
        PaintCommandArgument arg(PaintCommandArgument::Type::NUM, n);
        m_arguments.push_back(arg);
    }

    Command type()
    {
        return m_type;
    }

    std::vector<PaintCommandArgument>& arguments()
    {
        return m_arguments;
    }

private:
    Command m_type;
    // argument
    std::vector<PaintCommandArgument> m_arguments;
};

class PaintCommandBuffer {
public:
    PaintCommandBuffer();

    void insertCommand(PaintCommand c)
    {
        m_commands.push_back(c);
    }

    void clearCommnad()
    {
        m_commands.clear();
    }

    void paintCommands(Canvas*);

private:
    std::vector<PaintCommand> m_commands;
};
}

#endif
#endif
