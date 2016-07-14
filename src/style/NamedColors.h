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

#ifndef __StarFishNamedColors__
#define __StarFishNamedColors__

// https://developer.mozilla.org/en/docs/Web/CSS/color_value
#define NAMED_COLOR_FOR_EACH(F) \
    F(black   , 0x000000) \
    F(silver  , 0xc0c0c0) \
    F(gray    , 0x808080) \
    F(white   , 0xffffff) \
    F(maroon  , 0x800000) \
    F(red , 0xff0000) \
    F(purple  , 0x800080) \
    F(fuchsia , 0xff00ff) \
    F(green   , 0x008000) \
    F(lime    , 0x00ff00) \
    F(olive   , 0x808000) \
    F(yellow  , 0xffff00) \
    F(navy    , 0x000080) \
    F(blue    , 0x0000ff) \
    F(teal    , 0x008080) \
    F(aqua    , 0x00ffff) \
    F(orange  , 0xffa500) \
    F(aliceblue   , 0xf0f8ff) \
    F(antiquewhite    , 0xfaebd7) \
    F(aquamarine  , 0x7fffd4) \
    F(azure   , 0xf0ffff) \
    F(beige   , 0xf5f5dc) \
    F(bisque  , 0xffe4c4) \
    F(blanchedalmond  , 0xffebcd) \
    F(blueviolet  , 0x8a2be2) \
    F(brown   , 0xa52a2a) \
    F(burlywood   , 0xdeb887) \
    F(cadetblue   , 0x5f9ea0) \
    F(chartreuse  , 0x7fff00) \
    F(chocolate   , 0xd2691e) \
    F(coral   , 0xff7f50) \
    F(cornflowerblue  , 0x6495ed) \
    F(cornsilk    , 0xfff8dc) \
    F(crimson , 0xdc143c) \
    F(cyan , 0x00ffff) \
    F(darkblue    , 0x00008b) \
    F(darkcyan    , 0x008b8b) \
    F(darkgoldenrod   , 0xb8860b) \
    F(darkgray    , 0xa9a9a9) \
    F(darkgreen   , 0x006400) \
    F(darkgrey    , 0xa9a9a9) \
    F(darkkhaki   , 0xbdb76b) \
    F(darkmagenta , 0x8b008b) \
    F(darkolivegreen  , 0x556b2f) \
    F(darkorange  , 0xff8c00) \
    F(darkorchid  , 0x9932cc) \
    F(darkred , 0x8b0000) \
    F(darksalmon  , 0xe9967a) \
    F(darkseagreen    , 0x8fbc8f) \
    F(darkslateblue   , 0x483d8b) \
    F(darkslategray   , 0x2f4f4f) \
    F(darkslategrey   , 0x2f4f4f) \
    F(darkturquoise   , 0x00ced1) \
    F(darkviolet  , 0x9400d3) \
    F(deeppink    , 0xff1493) \
    F(deepskyblue , 0x00bfff) \
    F(dimgray , 0x696969) \
    F(dimgrey , 0x696969) \
    F(dodgerblue  , 0x1e90ff) \
    F(firebrick   , 0xb22222) \
    F(floralwhite , 0xfffaf0) \
    F(forestgreen , 0x228b22) \
    F(gainsboro   , 0xdcdcdc) \
    F(ghostwhite  , 0xf8f8ff) \
    F(gold    , 0xffd700) \
    F(goldenrod   , 0xdaa520) \
    F(greenyellow , 0xadff2f) \
    F(grey    , 0x808080) \
    F(honeydew    , 0xf0fff0) \
    F(hotpink , 0xff69b4) \
    F(indianred   , 0xcd5c5c) \
    F(indigo  , 0x4b0082) \
    F(ivory   , 0xfffff0) \
    F(khaki   , 0xf0e68c) \
    F(lavender    , 0xe6e6fa) \
    F(lavenderblush   , 0xfff0f5) \
    F(lawngreen   , 0x7cfc00) \
    F(lemonchiffon    , 0xfffacd) \
    F(lightblue   , 0xadd8e6) \
    F(lightcoral  , 0xf08080) \
    F(lightcyan   , 0xe0ffff) \
    F(lightgoldenrodyellow    , 0xfafad2) \
    F(lightgray   , 0xd3d3d3) \
    F(lightgreen  , 0x90ee90) \
    F(lightgrey   , 0xd3d3d3) \
    F(lightpink   , 0xffb6c1) \
    F(lightsalmon , 0xffa07a) \
    F(lightseagreen   , 0x20b2aa) \
    F(lightskyblue    , 0x87cefa) \
    F(lightslategray  , 0x778899) \
    F(lightslategrey  , 0x778899) \
    F(lightsteelblue  , 0xb0c4de) \
    F(lightyellow , 0xffffe0) \
    F(limegreen   , 0x32cd32) \
    F(linen   , 0xfaf0e6) \
    F(magenta   , 0xff00ff) \
    F(mediumaquamarine    , 0x66cdaa) \
    F(mediumblue  , 0x0000cd) \
    F(mediumorchid    , 0xba55d3) \
    F(mediumpurple    , 0x9370db) \
    F(mediumseagreen  , 0x3cb371) \
    F(mediumslateblue , 0x7b68ee) \
    F(mediumspringgreen   , 0x00fa9a) \
    F(mediumturquoise , 0x48d1cc) \
    F(mediumvioletred , 0xc71585) \
    F(midnightblue    , 0x191970) \
    F(mintcream   , 0xf5fffa) \
    F(mistyrose   , 0xffe4e1) \
    F(moccasin    , 0xffe4b5) \
    F(navajowhite , 0xffdead) \
    F(oldlace , 0xfdf5e6) \
    F(olivedrab   , 0x6b8e23) \
    F(orangered   , 0xff4500) \
    F(orchid  , 0xda70d6) \
    F(palegoldenrod   , 0xeee8aa) \
    F(palegreen   , 0x98fb98) \
    F(paleturquoise   , 0xafeeee) \
    F(palevioletred   , 0xdb7093) \
    F(papayawhip  , 0xffefd5) \
    F(peachpuff   , 0xffdab9) \
    F(peru    , 0xcd853f) \
    F(pink    , 0xffc0cb) \
    F(plum    , 0xdda0dd) \
    F(powderblue  , 0xb0e0e6) \
    F(rosybrown   , 0xbc8f8f) \
    F(royalblue   , 0x4169e1) \
    F(saddlebrown , 0x8b4513) \
    F(salmon  , 0xfa8072) \
    F(sandybrown  , 0xf4a460) \
    F(seagreen    , 0x2e8b57) \
    F(seashell    , 0xfff5ee) \
    F(sienna  , 0xa0522d) \
    F(skyblue , 0x87ceeb) \
    F(slateblue   , 0x6a5acd) \
    F(slategray   , 0x708090) \
    F(slategrey   , 0x708090) \
    F(snow    , 0xfffafa) \
    F(springgreen , 0x00ff7f) \
    F(steelblue   , 0x4682b4) \
    F(tan , 0xd2b48c) \
    F(thistle , 0xd8bfd8) \
    F(tomato  , 0xff6347) \
    F(turquoise   , 0x40e0d0) \
    F(violet  , 0xee82ee) \
    F(wheat   , 0xf5deb3) \
    F(whitesmoke  , 0xf5f5f5) \
    F(yellowgreen , 0x9acd32) \
    F(transparent , 0x000000)


#endif
