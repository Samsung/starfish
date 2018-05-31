using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace StarFishWindowsShell
{
    public partial class Console : Form
    {
        StarFish mStarFish;
        public Console(StarFish starFish)
        {
            mStarFish = starFish;
            InitializeComponent();
            webBrowserConsoleOutput.Navigate("about:blank");
            webBrowserConsoleOutput.Navigated += WebBrowserConsoleOutput_Navigated;
        }

        private void WebBrowserConsoleOutput_Navigated(object sender, WebBrowserNavigatedEventArgs e)
        {
            webBrowserConsoleOutput.Document.Body.Parent.Style = "margin : 0px";
            webBrowserConsoleOutput.Document.Body.Style = "font-family: sans-serif; overflow-y: scroll";
        }

        public void logMessge(string txt)
        {
            if (webBrowserConsoleOutput.Document != null)
            {
                HtmlElement div = webBrowserConsoleOutput.Document.CreateElement("div");
                div.InnerText = txt;
                div.Style = "padding: 2px 0px; border-bottom: 1px grey solid";
                webBrowserConsoleOutput.Document.Body.AppendChild(div);
                webBrowserConsoleOutput.Document.Window.ScrollTo(0, webBrowserConsoleOutput.Document.Body.ScrollRectangle.Height);
            }
        }

        void gotJSResult(String txt)
        {
            logMessge(txt);
        }

        private void textBoxConsoleInput_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
            {
                string txt = textBoxConsoleInput.Text;
                mStarFish.evaluateJS(txt, new StarFish.GotJSResult(gotJSResult));
                textBoxConsoleInput.Text = "";
                e.SuppressKeyPress = true;
            }
        }

        private void textBoxConsoleInput_TextChanged(object sender, EventArgs e)
        {

        }

        private void textBoxConsoleInput_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Enter)
            {
                e.Handled = true;
            }
        }
    }
}
