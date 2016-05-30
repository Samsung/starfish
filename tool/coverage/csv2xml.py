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
'	 <Style ss:ID="ColumnName">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'                <Border ss:Position="Left" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Right" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Top" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'            </Borders>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="RowName">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'                <Border ss:Position="Left" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'                <Border ss:Position="Right" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'                <Border ss:Position="Top" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'            </Borders>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="Body">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Left" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Right" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Top" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'            </Borders>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="Coverage">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'                <Border ss:Position="Left" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Right" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Top" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'            </Borders>' + "\n" + \
'            <Interior ss:Color=\'#C6EFCE\' ss:Pattern=\'Solid\'/>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="CoverageData">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Left" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Right" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Top" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'            </Borders>' + "\n" + \
'            <Interior ss:Color=\'#C6EFCE\' ss:Pattern=\'Solid\'/>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="ColumnCoverage">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'                <Border ss:Position="Left" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Right" ss:LineStyle="Continuous" ss:Weight="1" />' + "\n" + \
'                <Border ss:Position="Top" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'            </Borders>' + "\n" + \
'            <Interior ss:Color=\'#C6EFCE\' ss:Pattern=\'Solid\'/>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="EmptyRight">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Left" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'            </Borders>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="EmptyLeft">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Right" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'            </Borders>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="EmptyTop">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Bottom" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'            </Borders>' + "\n" + \
'        </Style>' + "\n" + \
'        <Style ss:ID="EmptyBottom">' + "\n" + \
'            <Borders>' + "\n" + \
'                <Border ss:Position="Top" ss:LineStyle="Continuous" ss:Weight="2" />' + "\n" + \
'            </Borders>' + "\n" + \
'        </Style>' + "\n" + \
'    </Styles>' + "\n")
xmlfile.write('    <Worksheet ss:Name="Coverage">' + "\n")

rowNum = 0
columnNum = 0
zero_flag = 0
zero = []
zero_index = []
first = []
second = []
third = []
across = []
across_zero = []
down = []

for row in reader:
    if rowNum == 0:
        first = row
	columnNum = len(row)
    if rowNum == 1:
        second = row
    # total coverage file
    if rowNum == 2 and row[1] == 'TC':
	zero = first
	first = second
	second = row
	zero_flag = 1
    rowNum +=1
    third.insert(rowNum, row[0])

j = 0
if zero_flag == 1:
    for i in range(len(zero)):
	if zero[i] != '':
	    zero_index.insert(j,i)
	    j += 1
zero_index.insert(j,-1)
zero_index.insert(j+1,-1)
zero_index.insert(j+2,-1)

#debug
#print zero_index
#print zero
#print len(zero)
#print first
#print len(first)
#print second
#print len(second)
#print third
#print len(third)
#debug

# column first
m = 1
j = 0
value = 0
for i in range(3,len(first)):
    if zero_flag == 1 and i == zero_index[m]:
	value = 0
	across.insert(j, value)
	m += 1
	j += 1
    elif first[i] == '':
	value += 1
	across.pop(j-1)
	across.insert(j-1, value)
    else:
	across.insert(j, 0)
	value = 0
	j += 1

# row first
j = 0
value = 0
if zero_flag == 0:
    start = 2
else:
    start = 3
for i in range(start, len(third)-1):
    if third[i] =='':
	value += 1
	down.pop(j-1)
        down.insert(j-1, value)
    else:
	down.insert(j,0)
	value = 0
	j += 1
down.insert(j, 0)

# column zero
j = 0
value = 0
if zero_flag == 1:
    for i in range(2, len(zero)):
        if zero[i] == '':
            value += 1
            across_zero.pop(j-1)
            across_zero.insert(j-1, value)
        else:
            across_zero.insert(j, 0)
            value = 0
            j += 1

#debug
#print across
#print down
#print across_zero
#debug

xmlfile.write('        <Table ss:ExpandedColumnCount="' + str(columnNum+2) + '" ss:ExpandedRowCount="' + str(rowNum+2) + '" x:FullColumns="1" x:FullRows="1">' + "\n")
csvfile.close()

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

csvfile = open(sys.argv[1], 'rb')
reader = csv.reader(csvfile, delimiter= '\t', quotechar='|')

# EmptyTop
xmlfile.write('            <Row>' + "\n")
for i in range(columnNum + 2):
    if i == 0 or i == columnNum + 1:
	xmlfile.write('                <Cell ss:StyleID="Default"></Cell>' + "\n")
    else:
        xmlfile.write('                <Cell ss:StyleID="EmptyTop"></Cell>' + "\n")
xmlfile.write('            </Row>' + "\n")

test = 0
acrossZeroTag = 0
acrossTag = 0
coverageLine = []
j = 0
k = 0 # coverageLine 
m = 0 # down
num = 0
u = 1
# write data
for row in reader:
    # EmptyLeft
    xmlfile.write('            <Row>'+"\n")
    xmlfile.write('                ' + '<Cell ss:StyleID="EmptyLeft" ss:Index="1"></Cell>' + "\n")
    for i in range(len(row)):
	# zero line exist
	if zero_flag == 1 and num == 0:
	    if acrossZeroTag != 0:
	        acrossZeroTag -= 1
	        continue
            if row[i] != '':
	        acrossZeroTag = across_zero[j]
	        j += 1
	    xmlfile.write('                ' + '<Cell ss:StyleID="ColumnName" ss:MergeAcross="' + str(acrossZeroTag) + '"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
	# first line
	elif (zero_flag == 1 and num == 1) or (zero_flag == 0 and num == 0):
            if acrossTag != 0:
		acrossTag -= 1
		continue
	    if i == zero_index[u]:
		u += 1
		acrossTag = across[j]
		j += 1
		xmlfile.write('                ' + '<Cell ss:StyleID="ColumnName" ss:MergeAcross="' + str(acrossTag) + '"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		continue
	    if row[i] != '':
		acrossTag = across[j]
		j += 1
	    xmlfile.write('                ' + '<Cell ss:StyleID="ColumnName" ss:MergeAcross="' + str(acrossTag) + '"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
	# second line
	elif (zero_flag == 1 and num == 2) or (zero_flag == 0 and num == 1):
            if row[i] == 'Coverage':
		xmlfile.write('                ' + '<Cell ss:StyleID="ColumnCoverage"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		continue
	    xmlfile.write('                ' + '<Cell ss:StyleID="ColumnName"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
	# data line
	else:
	    # mergedown
	    if i == 0 and row[i] != '':
		xmlfile.write('                ' + '<Cell ss:StyleID="RowName" ss:Index="2" ss:MergeDown="' + str(down[m]) + '"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		m += 1
	    elif i == 0 and row[i] == '':
		test += 1
		continue
	    # coverage row line
            elif row[1] == 'coverage' and i == 1:
		xmlfile.write('                ' + '<Cell ss:StyleID="Coverage" ss:Index="3"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
	    elif row[1] == 'coverage' and i > 1:
		xmlfile.write('                ' + '<Cell ss:StyleID="Coverage"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
	    # data line
	    else:
	        
		if i == 1:
		    xmlfile.write('                ' + '<Cell ss:StyleID="Body" ss:Index="3"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		    continue
		elif zero_flag == 1 and i == zero_index[k]:
		    xmlfile.write('                ' + '<Cell ss:StyleID="CoverageData"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		    k+=1
		    continue
		elif zero_flag == 0 and i == 2:
		    xmlfile.write('                ' + '<Cell ss:StyleID="CoverageData"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
		    continue
		xmlfile.write('                ' + '<Cell ss:StyleID="Body"><Data ss:Type="String">' + row[i] + '</Data></Cell>' + "\n")
    k = 0
    j = 0
    num += 1
    # EmptyRight
    xmlfile.write('                ' + '<Cell ss:StyleID="EmptyRight" ss:Index="' + str(columnNum + 2) + '"></Cell>' + "\n")
    xmlfile.write('            </Row>'+"\n")

# EmptyBottom
xmlfile.write('            <Row>' + "\n")
for i in range(columnNum + 2):
    if i == 0 or i == columnNum + 1:
        xmlfile.write('                <Cell ss:StyleID="Default"></Cell>' + "\n")
    else:
        xmlfile.write('                <Cell ss:StyleID="EmptyBottom"></Cell>' + "\n")
xmlfile.write('            </Row>' + "\n")
xmlfile.write('        </Table>' + "\n" + '    </Worksheet>' + "\n" + '</Workbook>' + "\n")

csvfile.close()
xmlfile.close()
