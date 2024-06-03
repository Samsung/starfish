/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "ShellConfig.h"
#include "UnitTestRunner.h"

#ifndef STARFISH_TIZEN
#include "gtest/gtest.h"
#endif

namespace StarfishShell {

UnitTestRunner::UnitTestRunner()
{
}

UnitTestRunner::~UnitTestRunner()
{
}

void UnitTestRunner::initialize(int argc, char* argv[])
{
#ifndef STARFISH_TIZEN
    testing::InitGoogleTest(&argc, argv);
#endif
}

int UnitTestRunner::runAllTests()
{
#ifndef STARFISH_TIZEN
    return testing::UnitTest::GetInstance()->Run();
#else
    return 0;
#endif
}
} // namespace StarfishShell
