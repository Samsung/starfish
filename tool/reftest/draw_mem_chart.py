import matplotlib.pyplot as plt
import sys
import StringIO
import urllib, base64
import os
from datetime import datetime

def drawChart(fileName):
    result=''
    idx = 0
    g_gc = []
    g_rss = []
    x = []
    with open(fileName) as f:
        for line in f:
            gc_mem, rss_mem = line.split()
            g_gc.append(gc_mem)
            g_rss.append(rss_mem)
            x.append(idx)
            idx = idx + 1
            # print(gc_mem, rss_mem)

    fig, axarr = plt.subplots(2, sharex=True)

    fig.suptitle(fileName, fontsize=14, fontweight='bold')

    axarr[0].plot(x, g_gc)
    axarr[0].set_title('GC')
    axarr[0].set_ylabel('MB')
    axarr[1].plot(x, g_rss)
    axarr[1].set_title('RSS')
    axarr[1].set_ylabel('MB')

    axarr[1].set_xlabel('GC Cnt')

    # fig.savefig(fileName[0:len(fileName)-4]+'.png')
    imgdata = StringIO.StringIO()
    fig.savefig(imgdata, format='png')
    imgdata.seek(0)
    uri = 'data:image/png;base64,' + urllib.quote(base64.b64encode(imgdata.buf))
    #print '<img src = "%s"/>' % uri
    result = '<img src = "'+uri+'"/>'
    # plt.show()
    plt.close(fig)
    return result


if __name__ == '__main__':
    result='''
    <html>
    <body>
    '''
    if len(sys.argv) == 1:
        print 'Plz, insert data file path'
        exit()

    for root, directories, filenames in os.walk(sys.argv[1]):
        for filename in filenames:
            if filename.endswith('.txt'):
                print filename
                result+=drawChart(root+filename)+"\n"

    result +='</body><html>'
    reulstFileName=datetime.today().strftime("%Y-%m-%d-%H-%M-%S")

    fp = open(reulstFileName+'.html', 'w')
    fp.write(result)
    fp.close()