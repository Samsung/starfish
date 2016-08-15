import matplotlib.pyplot as plt
import sys



if __name__ == '__main__':
    fileName=''
    g_gc = []
    g_rss = []
    x = []
    idx = 0
    if len(sys.argv) == 1:
        print 'Plz, insert data file path'
    else:
        fileName=sys.argv[1]


    with open(fileName) as f:
        for line in f:
            gc_mem, rss_mem = line.split()
            g_gc.append(gc_mem)
            g_rss.append(rss_mem)
            x.append(idx)
            idx=idx+1
            #print(gc_mem, rss_mem)

    fig, axarr = plt.subplots(2, sharex=True)

    fig.suptitle(fileName, fontsize=14, fontweight='bold')

    axarr[0].plot(x, g_gc)
    axarr[0].set_title('GC')
    axarr[0].set_ylabel('MB')
    axarr[1].plot(x, g_rss)
    axarr[1].set_title('RSS')
    axarr[1].set_ylabel('MB')

    axarr[1].set_xlabel('GC Cnt')

    fig.savefig(fileName[0:len(fileName)-4]+'.png')
    #plt.show()
    plt.close(fig)