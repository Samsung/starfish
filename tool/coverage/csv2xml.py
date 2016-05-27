#!/usr/bin/env python
import csv
import sys

if len(sys.argv) != 2:
    os._exit(1)

index = sys.argv[1].find('.')
xmlName = sys.argv[1][:index] + '.xml'

csvfile = open(sys.argv[1], 'rb')
xmlfile = open(xmlName, 'wb')
reader = csv.reader(csvfile, delimiter= '\t', quotechar='|')

xmlfile.write('<?xml version="1.0"?>' + "\n")
xmlfile.write('<?mso-application progid="Excel.Sheet"?>' + "\n")
xmlfile.write('<Workbook' + "\n" + \
'    xmlns="urn:schemas-microsoft-com:office:spreadsheet"' + "\n" + \
'    xmlns:o="urn:schemas-microsoft-com:office:office"' + "\n" +  \
'    xmlns:x="urn:schemas-microsoft-com:office:excel"' + "\n" + \
'    xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet"' + "\n" + \
'    xmlns:html="http://www.w3.org/TR/REC-html40">' + "\n")
xmlfile.write('    <DocumentProperties xmlns="urn:schemas-microsoft-com:office:office">' + "\n" + \
'    </DocumentProperties>' + "\n")
xmlfile.write('    <Styles>' + "\n" + \
'        <Style ss:ID="Default">' + "\n" + \
'            <Alignment ss:Horizontal="Left" />' + "\n" + \
'            <Font ss:Size="11" ss:Bold="0" />' + "\n" + \
'        </Style>' + "\n" + \
'	 <Style ss:ID="Name">' + "\n" + \
'	     <Alignment ss:Horizontal="Center" />' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="Coverage">' + "\n" + \
#'            <Borders>' + "\n" + \
#'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
#'            </Borders>' + "\n" + \
'            <Interior ss:Color=\'#C6EFCE\' ss:Pattern=\'Solid\'/>' + "\n" + \
'        </Style>' + "\n" + \
'    </Styles>' + "\n")
xmlfile.write('    <Worksheet ss:Name="Coverage">' + "\n")

rowNum = 0
columnNum = 0
first = []
second = []
third = []
across = []
down = []

for row in reader:
    if rowNum == 0:
        first = row
    if rowNum == 1:
        second = row
    rowNum +=1
    if len(row) > columnNum:
        columnNum = len(row)
    third.insert(rowNum, row[0])

j = 0
t = 0

for i in range(len(first), len(second)):
    first.insert(i,'' )
for i in range(3,len(second)):
    if first[i] == '':
	t += 1
	across.pop(j-1)
	across.insert(j-1, t)
    else:
	across.insert(j, 0)
	t = 0
	j += 1

j = 0
t = 0
for i in range(2, len(third)):
    if third[i] =='':
	t += 1
	down.pop(j-1)
        down.insert(j-1, t)
    else:
	down.insert(j,0)
	t = 0
	j += 1

down.insert(j-1, t-1)
down.insert(j, 0)
down.insert(j+1, 0)
xmlfile.write('        <Table ss:ExpandedColumnCount="' + str(columnNum) + '" ss:ExpandedRowCount="' + str(rowNum) + '" x:FullColumns="1" x:FullRows="1">' + "\n" + '            <Column ss:AutoFitWidth="0" ss:Width="100" />' + "\n")
csvfile.close()

csvfile = open(sys.argv[1], 'rb')
reader = csv.reader(csvfile, delimiter= '\t', quotechar='|')

acrossTag = 0
rowNum = 0
j = 0
k = 0
t = 0
for row in reader:
    if len(row) == 0:
    	xmlfile.write('            <Row>'+"\n"+'            </Row>'+"\n")
    else:
        xmlfile.write('            <Row>'+"\n")
        for i in range(len(row)):
	    if rowNum == 0:
		if acrossTag != 0:
		    acrossTag -= 1
		    continue
		if row[i] != '':
		    acrossTag = across[j]
		    j+=1
		xmlfile.write('                ' + '<Cell ss:StyleID="Name" ss:MergeAcross="' + str(acrossTag) + '"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
	    else:
		if row[i] == 'coverage':
		    xmlfile.write('		   ' + '<Cell ss:StyleID="Coverage" ss:Index="2"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		    t -= 1
	        elif (row[1] == 'coverage' or i == 2) and i != 0:
		    xmlfile.write('                ' + '<Cell ss:StyleID="Coverage"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		elif rowNum >= 2 and i == 0 and t == 0:
                    xmlfile.write('                ' + '<Cell ss:StyleID="Default" ss:Index="1" ss:MergeDown="' + str(down[k]) + '"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		    t = down[k] + 1
		    k += 1
		elif rowNum >= 2 and i == 0 and t != 0:
		    continue
		elif rowNum >= 2 and i == 1 and t != 0:
		    t -= 1
		    xmlfile.write('                ' + '<Cell ss:StyleID="Default" ss:Index="2"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		else:
		     xmlfile.write('                ' + '<Cell ss:StyleID="Default"><Data ss:Type="String">' + row[i] + '</Data>    </Cell>' + "\n")
	xmlfile.write("            </Row>"+"\n")
	rowNum += 1
xmlfile.write('        </Table>' + "\n" + '    </Worksheet>' + "\n" + '</Workbook>' + "\n")

csvfile.close()
xmlfile.close()
