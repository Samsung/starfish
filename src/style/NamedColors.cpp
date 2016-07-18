/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "NamedColors.h"

#define MATCH(name) if (memcmp(str, #name, sizeof(#name)) == 0) { ret = name##NamedColor; return true; }

bool parseNamedColor(const char* str, size_t length, NamedColorValue& ret)
{
    if (UNLIKELY(length == 0)) {
        return false;
    }

    char first = str[0];
    switch (first) {
    case 'a':
        if (length == 4) {
            MATCH(aqua);
        } else if (length == 5) {
            MATCH(azure);
        } else if (length == 9) {
            MATCH(aliceblue);
        } else if (length == 10) {
            MATCH(aquamarine);
        } else if (length == 12) {
            MATCH(antiquewhite);
        }
        break;
    case 'b':
        if (length == 4) {
            MATCH(blue);
        } else if (length == 5) {
            MATCH(black);
            MATCH(brown);
            MATCH(beige);
        } else if (length == 6) {
            MATCH(bisque);
        } else if (length == 9) {
            MATCH(burlywood);
        } else if (length == 10) {
            MATCH(blueviolet);
        } else if (length == 14) {
            MATCH(blanchedalmond);
        }

        break;
    case 'c':
        if (length == 4) {
            MATCH(cyan);
        } else if (length == 5) {
            MATCH(coral);
        } else if (length == 7) {
            MATCH(crimson);
        } else if (length == 8) {
            MATCH(cornsilk);
        } else if (length == 9) {
            MATCH(chocolate);
            MATCH(cadetblue);
        } else if (length == 10) {
            MATCH(chartreuse);
        } else if (length == 14) {
            MATCH(cornflowerblue);
        }
        break;
    case 'd':
        if (length == 7) {
            MATCH(dimgray);
            MATCH(dimgrey);
            MATCH(darkred);
        } else if (length == 8) {
            MATCH(darkblue);
            MATCH(darkcyan);
            MATCH(darkgray);
            MATCH(darkgrey);
            MATCH(deeppink);
        } else if (length == 9) {
            MATCH(darkgreen);
            MATCH(darkkhaki);
        } else if (length == 10) {
            MATCH(dodgerblue);
            MATCH(darkviolet);
            MATCH(darksalmon);
            MATCH(darkorange);
            MATCH(darkorchid);
        } else if (length == 11) {
            MATCH(deepskyblue);
            MATCH(darkmagenta);
        } else if (length == 12) {
            MATCH(darkseagreen);
        } else if (length == 13) {
            MATCH(darkgoldenrod);
            MATCH(darkslateblue);
            MATCH(darkslategray);
            MATCH(darkslategrey);
            MATCH(darkturquoise);
        } else if (length == 14) {
            MATCH(darkolivegreen);
        }
        break;
    case 'e':
        break;
    case 'f':
        if (length == 7) {
            MATCH(fuchsia);
        } else if (length == 9) {
            MATCH(firebrick);
        } else if (length == 11) {
            MATCH(floralwhite);
            MATCH(forestgreen);
        }

        break;
    case 'g':
        if (length == 4) {
            MATCH(gray);
            MATCH(grey);
            MATCH(gold);
        } else if (length == 5) {
            MATCH(green);
        } else if (length == 9) {
            MATCH(goldenrod);
            MATCH(gainsboro);
        } else if (length == 10) {
            MATCH(ghostwhite);
        } else if (length == 11) {
            MATCH(greenyellow);
        }

        break;
    case 'h':
        if (length == 7) {
            MATCH(hotpink);
        } else if (length == 8) {
            MATCH(honeydew);
        }
        break;
    case 'i':
        if (length == 5) {
            MATCH(ivory);
        } else if (length == 6) {
            MATCH(indigo);
        } else if (length == 9) {
            MATCH(indianred);
        }
        break;
    case 'j':
        break;
    case 'k':
        if (length == 5) {
            MATCH(khaki);
        }
        break;
    case 'l':
        if (length == 4) {
            MATCH(lime);
        } else if (length == 5) {
            MATCH(linen);
        } else if (length == 8) {
            MATCH(lavender);
        } else if (length == 9) {
            MATCH(lawngreen);
            MATCH(lightblue);
            MATCH(lightcyan);
            MATCH(lightgray);
            MATCH(lightgrey);
            MATCH(lightpink);
            MATCH(limegreen);
        } else if (length == 10) {
            MATCH(lightcoral);
            MATCH(lightgreen);
        } else if (length == 11) {
            MATCH(lightsalmon);
            MATCH(lightyellow);
        } else if (length == 12) {
            MATCH(lemonchiffon);
            MATCH(lightskyblue);
        } else if (length == 13) {
            MATCH(lavenderblush);
            MATCH(lightseagreen);
        } else if (length == 14) {
            MATCH(lightsteelblue);
            MATCH(lightslategray);
            MATCH(lightslategrey);
        } else if (length == 20) {
            MATCH(lightgoldenrodyellow);
        }
        break;
    case 'm':
        if (length == 6) {
            MATCH(maroon)
        } else if (length == 7) {
            MATCH(magenta);
        } else if (length == 8) {
            MATCH(moccasin);
        } else if (length == 9) {
            MATCH(mintcream);
            MATCH(mistyrose);
        } else if (length == 10) {
            MATCH(mediumblue);
        } else if (length == 12) {
            MATCH(midnightblue);
            MATCH(mediumorchid);
            MATCH(mediumpurple);
        } else if (length == 14) {
            MATCH(mediumseagreen);
        } else if (length == 15) {
            MATCH(mediumslateblue);
            MATCH(mediumturquoise);
            MATCH(mediumvioletred);
        } else if (length == 16) {
            MATCH(mediumaquamarine);
        } else if (length == 17) {
            MATCH(mediumspringgreen);
        }
        break;
    case 'n':
        if (length == 4) {
            MATCH(navy);
        } else if (length == 11) {
            MATCH(navajowhite);
        }

        break;
    case 'o':
        if (length == 5) {
            MATCH(olive);
        } else if (length == 6) {
            MATCH(orange);
            MATCH(orchid);
        } else if (length == 7) {
            MATCH(oldlace);
        } else if (length == 9) {
            MATCH(olivedrab);
            MATCH(orangered);
        }
        break;
    case 'p':
        if (length == 4) {
            MATCH(pink);
            MATCH(peru);
            MATCH(plum);
        } else if (length == 6) {
            MATCH(purple);
        } else if (length == 9) {
            MATCH(peachpuff);
            MATCH(palegreen);
        } else if (length == 10) {
            MATCH(powderblue);
            MATCH(papayawhip);
        } else if (length == 13) {
            MATCH(palegoldenrod);
            MATCH(paleturquoise);
            MATCH(palevioletred);
        }
        break;
    case 'q':
        break;
    case 'r':
        if (length == 3) {
            MATCH(red);
        } else if (length == 9) {
            MATCH(rosybrown);
            MATCH(royalblue);
        }
        break;
    case 's':
        if (length == 4) {
            MATCH(snow);
        } else if (length == 6) {
            MATCH(silver);
            MATCH(sienna);
            MATCH(salmon);
        } else if (length == 7) {
            MATCH(skyblue);
        } else if (length == 8) {
            MATCH(seashell);
            MATCH(seagreen);
        } else if (length == 9) {
            MATCH(slateblue);
            MATCH(slategray);
            MATCH(slategrey);
            MATCH(steelblue);
        } else if (length == 10) {
            MATCH(sandybrown);
        } else if (length == 11) {
            MATCH(springgreen);
            MATCH(saddlebrown);
        }
        break;
    case 't':
        if (length == 3) {
            MATCH(tan);
        } else if (length == 4) {
            MATCH(teal);
        } else if (length == 6) {
            MATCH(tomato);
        } else if (length == 7) {
            MATCH(thistle);
        } else if (length == 9) {
            MATCH(turquoise);
        } else if (length == 11) {
            MATCH(transparent);
        }
        break;
    case 'u':
        break;
    case 'v':
        if (length == 6) {
            MATCH(violet);
        }
        break;
    case 'w':
        if (length == 5) {
            MATCH(white);
            MATCH(wheat);
        } else if (length == 10) {
            MATCH(whitesmoke);
        }
        break;
    case 'x':
        break;
    case 'y':
        if (length == 6) {
            MATCH(yellow)
        } else if (length == 11) {
            MATCH(yellowgreen)
        }
        break;
    case 'z':
        break;
    }

    return false;
}
