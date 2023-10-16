using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace StarfishWinformShell
{
    public partial class DemoForm : Form
    {
        struct Data {
            public string url;
            public int timeout;
            public Data(string url, int timeout)
            {
                this.url = url;
                this.timeout = timeout;
            }
        }
        ArrayList mSiteList;
        int mIdx;
        Starfish mStarFish;
        public DemoForm(Starfish starFish)
        {
            InitializeComponent();
            mStarFish = starFish;

            int defaultTimeout = 15000;
            mSiteList = new ArrayList();
            mSiteList.Add(new Data("https://css-tricks.com/examples/Circulate/", defaultTimeout));
            mSiteList.Add(new Data("http://demo.tutorialzine.com/2010/02/photo-shoot-css-jquery/demo.html", defaultTimeout));
            mSiteList.Add(new Data("https://tympanus.net/Tutorials/BubbleNavigation/", defaultTimeout));
            mSiteList.Add(new Data("https://tympanus.net/Tutorials/BeautifulBackgroundImageNavigation/", defaultTimeout));
            mSiteList.Add(new Data("http://demo.tutorialzine.com/2010/06/apple-like-retina-effect-jquery-css/demo.html", defaultTimeout));
            mSiteList.Add(new Data("http://naver.com", defaultTimeout));
            mSiteList.Add(new Data("http://tistory.com", defaultTimeout));
            mSiteList.Add(new Data("https://en.wikipedia.org/wiki/Samsung", defaultTimeout));
            mSiteList.Add(new Data("http://bwikbs.github.io/PTAlarm/", defaultTimeout));
            mSiteList.Add(new Data("http://youtube.com/tv", 20 * 1000));
        }

        private void buttonStart_Click(object sender, EventArgs e)
        {
            Hide();
            mIdx = 0;
            Data d = ((Data)mSiteList[mIdx++]);
            mStarFish.Navigate(d.url);

            timerDemo.Interval = d.timeout;
            timerDemo.Start();
            timerDemo.Tick += TimerDemo_Tick;
        }

        private void TimerDemo_Tick(object sender, EventArgs e)
        {
            Data d = ((Data)mSiteList[mIdx++]);
            mStarFish.Navigate(d.url);
            if (mIdx == mSiteList.Count)
            {
                mIdx = 0;
            }
            timerDemo.Stop();
            timerDemo.Interval = d.timeout;
            timerDemo.Start();
        }
    }
}
