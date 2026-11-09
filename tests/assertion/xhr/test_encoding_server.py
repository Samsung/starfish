#!/usr/bin/python
# -*- coding: utf-8 -*-
import time
import BaseHTTPServer
import base64
import json

PORT_NUMBER = 8081
HOST_NAME = ''
textParameter=[]

RESPONSE_TYPE_DATA=['plain','json','html']
RESPONSE_STRING_LIST=['helloworld','helloworld_kor','lorem','lorem_cyrill','lorem_greek','lorem_hindi','lorem_ch','lorem_jpn','lorem_arabic'
                        ,'lorem_hebrew','lorem_luxemburg','lorem_esperanto','lorem_esperanto','lorem_quenya','lorem_volapuk'
                    ]
RESPONSE_STRING_DATA=[
                        '''Hello World!'''.decode('utf-8'),
                        '''헬로우 월드!'''.decode('utf-8'),
                        '''Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam ligula sapien, rutrum sed vestibulum eget, rhoncus ac erat. Aliquam erat volutpat. Sed convallis scelerisque enim at fermentum. Aliquam consectetur, est ac auctor iaculis, odio mi bibendum leo, in congue neque velit vel enim. Nullam vitae justo at mauris sodales feugiat. Praesent pellentesque ipsum eget tellus imperdiet ultrices. Sed ultricies nisi nec diam sodales fringilla. Quisque adipiscing cursus porta. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam bibendum scelerisque elit, eu pharetra dui pulvinar eget. Nam mollis mauris id tellus ultricies at porttitor neque vulputate. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos.'''.decode('utf-8'),
                        '''Эю мэль квюод конвынёры альиквуандо. Ыт квюот квуёдам фабулаз еюж, апыирёан торквюатоз экз прё. Аффэрт ийжквюы ыкчпэтында еюж но, ат зючкёпит ывыртятюр майыжтатйж эож. Эю хаж вэре дйкит, харюм нюмквуам эррорибуз йн эжт. Эа чингюльищ пэртинакёа конжыквуюнтюр нам, эи шэа конгуы ыкжпэтэндяз интыллыгам, эжт льаборэ антеопам ыт.'''.decode('utf-8'),
                        '''Φις αδ σριπθα ασυσαμυς σριπτορεμ, συ γραεσι ασομμοδαρε φιξ. Μανδαμυς δισεντιυνθ φιξ νο. Νο κυο σλιτα αλβυσιυς, μαιορυμ φυισετ σωνσλυδαθυρκυε μελ ατ, θαθιων περισυλα φελ ιδ. Υσυ αν πωσθεα ιυδισαβιτ, σριπτορεμ σριβενθυρ ινθερπρεταρις ει νες, σιθ εσεντ δολωρυμ αν. Εα πριμα λιβρις ιισκυε φις.'''.decode('utf-8'),
                        '''सहित बढाता शुरुआत तकनीकी देकर मजबुत व्रुद्धि प्रव्रुति प्रौध्योगिकी चिदंश दोषसके असरकारक निरपेक्ष खण्ड मेमत ध्येय भारतीय विज्ञान सुस्पश्ट नीचे शुरुआत शारिरिक केन्द्रित ध्वनि सुचना सेऔर हुआआदी विज्ञान प्रव्रुति दर्शाता केन्द्रिय विशेष कलइस सुचना सोफ़्टवेर अन्तरराष्ट्रीयकरन लिये दिये वास्तव विकेन्द्रित बाजार'''.decode('utf-8'),
                        '''孫望選軍育夫目項出能畑正。補入透実務時転係力同約都江理発売出題延見。社専刃百平三政式件批発年公痢。度吹無禁意情塊内月優際帰道誇石。掲穀者千坊観岸確中千催献路川時。性遣米対投特館見言産失済年青宅。響沖並以守全小案育司要子亡高洋。図車党帯村図庁者域肩年稿。扱保一呼辞気田誠佐換計覚砕家築。火向崎送実自外区散成記新囲物知。'''.decode('utf-8'),
                        '''責メヘ成直周クユイニ覧安トツ禁8提シ法町モヒノ南回おスト盤獲ヘケ西子動をン文予方ぱ械十ーぐ。西海著ルヌ張名ぽぶ巣店ずをお宮集づずやと気川つ図平や禁売とそさを点択タセノ合図コタチ社廃マ着治載あたぼ鳴無短トぶてれ。52氷べ語該通ヲ習看ぼあリだ要外レ条自ざめ助後社験ぴずけお記今にみおこ替軒カムロト断大ツウハ鈴4格フい無渡ぶをぞ悪馬ヒリセチ稿写モチラト国2費抱診露ぐラフぱ。'''.decode('utf-8'),
                        '''فعل ان فبعد مليون التغييرات, غير لم الثقيلة البشريةً. إذ سبتمبر التقليدي تحت. بلا الشمال واندونيسيا، أن, بين وبدأت قائمة الصفحة كل. يعبأ إنطلاق الخاطفة و مدن, مكن فسقط تسمّى المتحدة إذ. بوابة واُسدل قد عدم. إذ الفترة وانتهاءً وفي, ولم دارت عسكرياً المؤلّفة إذ. شيء للصين والنفيس بالسيطرة تم.'''.decode('utf-8'),
                        '''בקלות קלאסיים או שכל. מה לציין קלאסיים בהיסטוריה תנך, של תנך חופשית לעברית. ב היא לערך מרצועת, תנך של אתנולוגיה העריכהגירסאות. צעד היום כלשהו תרבות את. את בקר היום שתפו קלאסיים. מדעי פסיכולוגיה עוד ב.'''.decode('utf-8'),
                        '''Mir si Léift Blieder. En gin wait d'Sonn, iwer d'Mier heemlech ze der, um dir laacht Kirmesdag. Kaffi d'Liewen Nuechtegall vun ké. Wa dén Welt ma'n fest.'''.decode('utf-8'),
                        '''Be des gibi mebi. Mem plus norde emfazado go, nk tebi vavo neniaĵo pre. En avio trema solstariva unu, iz tien triliono suplemento kie, eks bv milo poŭpo maksimume. Io veo plua anstataŭe.'''.decode('utf-8'),
                        '''Holmë salmë né yat, an not palla palúrë. Ai men talan sanda ciryapanda, yá línë lauta lindë ser, venë arca ondo cir et. Vírë olosta lú yav. Nís up ilma vacco.'''.decode('utf-8'),
                        '''Ols fo ologons valiks, sui eperon kludo gö. Eflapom koldik yul va, klotis koldik foi ya. Val binonöd seväriko lo, tab di bethania eprofetobs-li predölo. Egivom-li oflapons ogidükons ons dö. Ek lananis menamödotis fat, mal klifs tidäbs ziläk of, ol sukols süls beg.'''.decode('utf-8'),
                        '''Ek ofs donio glävi sevobs, fenols sevob dö oms, plä om kristus odunob-li. Ni leitik padakipön mel, domi fasilikum kiom ol ini. Benosmelik nemögik neodols lio tü, bligi votik ata vo. Cüg dü diabis sagölo, om klop vemo ods. Bibiinolavanes detaflanü ebalädikol-li mot do, ome is gospul taledi.'''.decode('utf-8')
                      ]

def ParsingArgument(arg):

    RESPONSE_TYPE = 0
    RESPONSE_ENCODING = ''
    RESPONSE_STRING = 0
    RESPONSE_SIZE = 1

    print arg
    textParameter = arg.split('/')

    for index, val in enumerate(RESPONSE_TYPE_DATA):
        if val == textParameter[1]:
            RESPONSE_TYPE = index


    RESPONSE_ENCODING = textParameter[2]

    for index, val in enumerate(RESPONSE_STRING_LIST):
        if val == textParameter[3]:
            RESPONSE_STRING = index

    RESPONSE_SIZE = int(textParameter[4])

    print "RESPONSE_TYPE:"+str(RESPONSE_TYPE)
    print "RESPONSE_ENCODING:" + str(RESPONSE_ENCODING)
    print "RESPONSE_STRING:" + str(RESPONSE_STRING)
    print "RESPONSE_SIZE:" + str(RESPONSE_SIZE)

    return [RESPONSE_TYPE,RESPONSE_ENCODING,RESPONSE_STRING,RESPONSE_SIZE]


def makeResult(parsed_arg):
    RESPONSE_TYPE = parsed_arg[0]
    RESPONSE_ENCODING = parsed_arg[1]
    RESPONSE_STRING = parsed_arg[2]
    RESPONSE_SIZE = parsed_arg[3]

    result = ''

    tmp_str = RESPONSE_STRING_DATA[RESPONSE_STRING].encode(RESPONSE_ENCODING)

    #json
    if RESPONSE_TYPE==1:
        result = "json"
        json_data = {}
        for i in range(0, RESPONSE_SIZE):
            json_data['data'+str(i)] = tmp_str;
        result = json.dumps(json_data, ensure_ascii=False,indent=4)
    #html
    elif RESPONSE_TYPE==2:
        result='<!DOCTYPE html><html><body>'
        for i in range(0, RESPONSE_SIZE):
            result = result + tmp_str +'<br/>'
        result = result + '</body></html>'

    #plain
    else:
        for i in range(0, RESPONSE_SIZE):
            result = result + tmp_str

    return result

class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_HEAD(s):
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()

    def do_GET(s):

        if s.path == '/favicon.ico':
            return

        parsed_arg = ParsingArgument(s.path)
        s.send_response(200)
        s.send_header('Content-Type', 'text/html; charset='+parsed_arg[1])
        s.end_headers()
        tmp = makeResult(parsed_arg)
        s.wfile.write(tmp);


if __name__ == '__main__':

    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
    print time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER)


