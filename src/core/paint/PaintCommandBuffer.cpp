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

#include "StarFishConfig.h"
#include "core/paint/PaintCommandBuffer.h"

namespace StarFish {
PaintCommandBuffer::PaintCommandBuffer()
{
}

static void moveToFor2D(Canvas* canvas,
                        std::vector<PaintCommandArgument>& arguments)
{
    if (arguments.size() != 2) {
        return;
    }

    canvas->moveTo(arguments[0].value(), arguments[1].value());
}

static void lineToFor2D(Canvas* canvas,
                        std::vector<PaintCommandArgument>& arguments)
{
    if (arguments.size() != 2) {
        return;
    }

    canvas->lineTo(arguments[0].value(), arguments[1].value());
}

static void fillRectFor2D(Canvas* canvas,
                          std::vector<PaintCommandArgument>& arguments)
{
    if (arguments.size() != 4) {
        return;
    }

    canvas->drawRect(LayoutRect(arguments[0].value(), arguments[1].value(),
                                arguments[2].value(), arguments[3].value()));
    canvas->fill();
}

static void bezierCurveFor2D(Canvas* canvas,
                             std::vector<PaintCommandArgument>& arguments)
{
    if (arguments.size() != 6) {
        return;
    }

    canvas->curveTo(arguments[0].value(), arguments[1].value(),
                    arguments[2].value(), arguments[3].value(),
                    arguments[4].value(), arguments[5].value());
}

static void setLineWidhtFor2D(Canvas* canvas,
                              std::vector<PaintCommandArgument>& arguments)
{
    if (arguments.size() != 1) {
        return;
    }
    canvas->setStrokeWidth(arguments[0].value());
}

static void clearRectFor2D(Canvas* canvas,
                           std::vector<PaintCommandArgument>& arguments)
{
    if (arguments.size() != 4) {
        return;
    }
    canvas->setColor(Unit::Color(255, 255, 255, 255));
    canvas->drawRect(LayoutRect(arguments[0].value(), arguments[1].value(),
                                arguments[2].value(), arguments[3].value()));
    canvas->fill();
    canvas->setColor(Unit::Color(0, 0, 0, 255));
}

static void setColorFor2D(Canvas* canvas,
                          std::vector<PaintCommandArgument>& arguments)
{
    if (arguments.size() != 4) {
        return;
    }
    double r = arguments[0].value();
    double g = arguments[1].value();
    double b = arguments[2].value();
    double a = arguments[3].value();
    canvas->setColor(Unit::Color(r, g, b, a));
}

void PaintCommandBuffer::paintCommands(Canvas* canvas)
{
    if (!m_commands.size()) {
        return;
    }

    canvas->save();
    size_t numberOfSave = 0;
    // Set defulat value
    canvas->setStrokeWidth(1);
    for (auto& command : m_commands) {
        switch (command.type()) {
        case PaintCommand::Command::MOVETO_2D:
            moveToFor2D(canvas, command.arguments());
            break;
        case PaintCommand::Command::LINETO_2D:
            lineToFor2D(canvas, command.arguments());
            break;
        case PaintCommand::Command::STROKE_2D:
            canvas->stroke();
            break;
        case PaintCommand::Command::FILLRECT_2D:
            fillRectFor2D(canvas, command.arguments());
            break;
        case PaintCommand::Command::BEZIERCURVETO_2D:
            bezierCurveFor2D(canvas, command.arguments());
            break;
        case PaintCommand::Command::SETLINEWIDTH_2D:
            setLineWidhtFor2D(canvas, command.arguments());
            break;
        case PaintCommand::Command::CLEARRECT_2D:
            clearRectFor2D(canvas, command.arguments());
            break;
        case PaintCommand::Command::SAVE_2D:
            numberOfSave++;
            canvas->save();
            break;
        case PaintCommand::Command::RESTORE_2D:
            if (numberOfSave) {
                canvas->restore();
                numberOfSave--;
            }
            break;
        case PaintCommand::Command::SETCOLOR_2D:
            setColorFor2D(canvas, command.arguments());
            break;
        default:
            break;
        }
    }

    for (size_t count = 0; count < numberOfSave; count++) {
        canvas->restore();
    }
    canvas->restore();
    m_commands.clear();
}
}
#endif
