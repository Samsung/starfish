using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Windows.Interop;

namespace StarFishWindowsShell
{
    public partial class BrowserUIForm : Form, IMessageFilter
    {
        StarFish mStarFish;
        Console mConsole;
        int mPixelRatio = 1;
        string mInitialTitle;
        public BrowserUIForm()
        {
            InitializeComponent();
            this.DoubleBuffered = true;
            string[] args = Environment.GetCommandLineArgs();
            string url = "about:blank";
            if (args.Length > 1)
            {
                url = args[1];
                if (!url.Contains("://"))
                {
                    url = "file:///" + Path.GetFullPath(url).Replace("\\\\", "\\").Replace("\\", "/");
                }
            }
            textBoxAddress.Text = url;
            pictureBoxBrowserContent.MouseWheel += PictureBoxBrowserContent_MouseWheel;
            mStarFish = new StarFish(this, pictureBoxBrowserContent.Width, pictureBoxBrowserContent.Height, url);
            mStarFish.OnScreenBufferUpdate = new StarFish.ScreenBufferUpdated(onUpdateScreenBitmapDelegate);
            mStarFish.OnLoadPageStart = new StarFish.LoadPageStart(onLoadPageStartDelegate);
            mStarFish.OnGotMessage = new StarFish.GotMessage(onGotMessage);
            mInitialTitle = this.Text;

            Application.AddMessageFilter(this);
        }

        // P/Invoke declarations
        [DllImport("user32.dll")]
        private static extern IntPtr WindowFromPoint(Point pt);
        [DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wp, IntPtr lp);
        public bool PreFilterMessage(ref Message m)
        {
            if (m.Msg == (int)0x20a)
            {
                // WM_MOUSEWHEEL, find the control at screen position m.LParam
                var hWnd = WindowFromPoint(Cursor.Position);

                if (hWnd != IntPtr.Zero && hWnd != m.HWnd && Control.FromHandle(hWnd) != null)
                {
                    SendMessage(hWnd, m.Msg, m.WParam, m.LParam);
                    return true;
                }
            }

            return false;
        }

        private void MConsole_FormClosing(object sender, FormClosingEventArgs e)
        {
            mConsole = null;
        }

        public void onUpdateScreenBitmapDelegate(Bitmap bitmap)
        {
            if (pictureBoxBrowserContent.Image != bitmap)
            {
                using (Image old = pictureBoxBrowserContent.Image)
                {
                    pictureBoxBrowserContent.Image = bitmap;
                }
            }
        }

        public void onGotMessage(string msg, StarFish.MessageKind kind)
        {
            if (mConsole != null)
            {
                mConsole.logMessge(msg);
            }
        }

        private void BrowserUIForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            mStarFish.Close();
        }

        private void BrowserUIForm_Resize(object sender, EventArgs e)
        {
            if (this.WindowState != FormWindowState.Minimized)
            {
                mStarFish.Resize(pictureBoxBrowserContent.Width * mPixelRatio, pictureBoxBrowserContent.Height * mPixelRatio);
                this.Text = mInitialTitle + " " + pictureBoxBrowserContent.Width * mPixelRatio + "x" + pictureBoxBrowserContent.Height * mPixelRatio + "(x" + (1.0 / mPixelRatio) + ")";
            }
        }

        private void textBoxAddress_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
            {
                mStarFish.Navigate(textBoxAddress.Text);
                e.SuppressKeyPress = true;
            }
        }

        private void buttonNavigate_Click(object sender, EventArgs e)
        {
            mStarFish.Navigate(textBoxAddress.Text);
        }

        private void pictureBoxBrowserContent_MouseMove(object sender, MouseEventArgs e)
        {
            mStarFish.dispatchMouseMoveEvent(e.X * mPixelRatio, e.Y * mPixelRatio, e.Button.HasFlag(MouseButtons.Left), e.Button.HasFlag(MouseButtons.Right));
        }

        private void pictureBoxBrowserContent_MouseUp(object sender, MouseEventArgs e)
        {
            mStarFish.dispatchMouseUpEvent(e.X * mPixelRatio, e.Y * mPixelRatio);
        }

        private void pictureBoxBrowserContent_MouseDown(object sender, MouseEventArgs e)
        {
            mStarFish.dispatchMouseDownEvent(e.X * mPixelRatio, e.Y * mPixelRatio);
            textBoxFocusReceiver.Focus();
        }

        private void pictureBoxBrowserContent_MouseEnter(object sender, EventArgs e)
        {
            textBoxFocusReceiver.Focus();
        }

        private void pictureBoxBrowserContent_MouseLeave(object sender, EventArgs e)
        {
        }

        private void PictureBoxBrowserContent_MouseWheel(object sender, MouseEventArgs e)
        {
            mStarFish.dispatchMouseWheelEvent(e.X * mPixelRatio, e.Y * mPixelRatio, -e.Delta / 120);
        }

        private void onLoadPageStartDelegate(string url)
        {
            if (!textBoxAddress.Focused)
            {
                textBoxAddress.Text = url;
            }
        }

        private void buttonConsole_Click(object sender, EventArgs e)
        {
            if (mConsole == null)
            {
                mConsole = new Console(mStarFish);
                mConsole.Owner = this;
                mConsole.Show();
                mConsole.FormClosing += MConsole_FormClosing;
            }
            else
            {
                mConsole.Close();
                mConsole.Dispose();
                mConsole = null;
            }
        }

        private void textBoxAddress_KeyDown(object sender, KeyEventArgs e)
        {

        }

        private void textBoxAddress_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Enter)
            {
                e.Handled = true;
            }
        }

        private void buttonResizeFHDHalf_Click(object sender, EventArgs e)
        {
            int w = 1920 / 2;
            int h = 1080 / 2;
            mPixelRatio = 2;
            resizeInto(w, h);
        }

        private void buttonResizeFHD_Click(object sender, EventArgs e)
        {
            int w = 1920;
            int h = 1080;
            mPixelRatio = 1;
            resizeInto(w, h);
        }

        void resizeInto(int w, int h)
        {
            if (pictureBoxBrowserContent.Width < w)
            {
                this.Width = this.Width + (w - pictureBoxBrowserContent.Width);
            }

            if (pictureBoxBrowserContent.Width > w)
            {
                this.Width = this.Width - (pictureBoxBrowserContent.Width - w);
            }

            if (pictureBoxBrowserContent.Height < h)
            {
                this.Height = this.Height + (h - pictureBoxBrowserContent.Height);
            }

            if (pictureBoxBrowserContent.Height > h)
            {
                this.Height = this.Height - (pictureBoxBrowserContent.Height - h);
            }
        }

        private void BrowserUIForm_Load(object sender, EventArgs e)
        {
            buttonResizeFHDHalf.PerformClick();
        }

        private void BrowserUIForm_KeyDown(object sender, KeyEventArgs e)
        {
        }

        private void BrowserUIForm_KeyUp(object sender, KeyEventArgs e)
        {
        }

        private void textBoxFocusReceiver_KeyDown(object sender, KeyEventArgs e)
        {
            mStarFish.dispatchKeyDownEvent(e.KeyCode);
        }

        private void textBoxFocusReceiver_KeyUp(object sender, KeyEventArgs e)
        {
            mStarFish.dispatchKeyUpEvent(e.KeyCode);
        }

        private void pictureBoxBrowserContent_Click(object sender, EventArgs e)
        {

        }

        private void BrowserUIForm_DragDrop(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
                for (int i = 0; i < files.Length; i ++)
                {
                    if (files[i].EndsWith(".html") || files[i].EndsWith(".htm"))
                    {
                        string url = "file:///" + files[i];
                        url = url.Replace('\\', '/');
                        mStarFish.Navigate(url);
                        break;
                    }
                }
            }
        }

        private void BrowserUIForm_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = DragDropEffects.Link;
        }
    }


    public class StarFish
    {
        [DllImport("StarFish.dll")]
        public static extern IntPtr createWebViewInstance(uint initialWidth,
                                             uint initialHeight, IntPtr initialBuffer, uint initialBufferStride);
        [DllImport("StarFish.dll")]
        public static extern void loadURL(IntPtr mWebViewInstance, IntPtr utf8URL, uint urlBufferLength);
        [DllImport("StarFish.dll")]
        public static extern void giveMessage(IntPtr mWebViewInstance, MSG msg);
        [DllImport("StarFish.dll")]
        public static extern uint updateDrawingBufferAddress(IntPtr mWebViewInstance, uint width, uint height, IntPtr buffer, uint stride);
        [DllImport("StarFish.dll")]
        public static extern void resizeWindow(IntPtr mWebViewInstance, uint width, uint height, IntPtr buffer, uint stride);
        [DllImport("StarFish.dll")]
        public static extern uint drawingBufferFrameNumber(IntPtr mWebViewInstance);
        [DllImport("StarFish.dll")]
        public static extern void dispatchMouseDownEvent(IntPtr mWebViewInstance, float x, float y);
        [DllImport("StarFish.dll")]
        public static extern void dispatchMouseUpEvent(IntPtr mWebViewInstance, float x, float y);
        [DllImport("StarFish.dll")]
        public static extern void dispatchMouseMoveEvent(IntPtr mWebViewInstance, float x, float y, bool isLButtonPressed, bool isRButtonPressed);
        [DllImport("StarFish.dll")]
        public static extern void dispatchMouseWheelEvent(IntPtr mWebViewInstance, float x, float y, int delta);
        [DllImport("StarFish.dll")]
        public static extern void dispatchKeyDownEvent(IntPtr mWebViewInstance, uint keyCode);
        [DllImport("StarFish.dll")]
        public static extern void dispatchKeyUpEvent(IntPtr mWebViewInstance, uint keyCode);

        public struct EvaluateJSResult
        {
            public IntPtr buf;
            public uint len;
        };
        [return: MarshalAs(UnmanagedType.Struct)]
        [DllImport("StarFish.dll")]
        public static extern EvaluateJSResult evaluateJS(IntPtr mWebViewInstance, IntPtr utf8Code, uint len);
        public struct MSG
        {
            public IntPtr hwnd { get; set; }
            public int message { get; set; }
            public IntPtr wParam { get; set; }
            public IntPtr lParam { get; set; }
            public int time { get; set; }
            public int pt_x { get; set; }
            public int pt_y { get; set; }
        }
        [DllImport("user32.dll")]
        static extern int GetMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin,
           uint wMsgFilterMax);
        [DllImport("kernel32.dll", EntryPoint = "CopyMemory", SetLastError = false)]
        public static extern void CopyMemory(IntPtr dest, IntPtr src, uint count);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr LocalFree(IntPtr hMem);

        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool PostThreadMessage(uint threadId, uint msg, UIntPtr wParam, IntPtr lParam);

        [DllImport("kernel32.dll")]
        static extern uint GetCurrentThreadId();
        public delegate void ScreenBufferUpdated(Bitmap bitmap);
        public delegate void LoadPageStart(string url);

        public enum MessageKind
        {
            Info, Error, Warn
        }
        public delegate void GotMessage(string txt, MessageKind kind);
        public delegate void GotJSResult(string txt);
        public delegate void CloseWaitingForm(Form form);

        BrowserUIForm mForm;
        IntPtr mWebViewInstance;
        Thread mStarFishThread;
        uint mStarFishThreadID;
        Bitmap mScreenBitmap;
        BitmapData mScreenBitmapData;
        uint mLastFrameNumber = uint.MaxValue;
        ArrayList mJSResultCallbackList;
        StarFishFontInitWaitForm mWaitingForm;

        public ScreenBufferUpdated OnScreenBufferUpdate { get; set; }
        public LoadPageStart OnLoadPageStart { get; set; }
        public GotMessage OnGotMessage { get; set; }

        struct StartArg
        {
            public int initialWidth;
            public int initialHeight;
            public string initialURL;
        }

        public StarFish(BrowserUIForm form, int initialWidth, int initialHeight, string initialURL)
        {
            mForm = form;
            mJSResultCallbackList = new ArrayList();
            mStarFishThread = new Thread(new ParameterizedThreadStart(this.WorkThread), 1024 * 1024 * 4);
            StartArg arg = new StartArg();
            arg.initialWidth = initialWidth;
            arg.initialHeight = initialHeight;
            arg.initialURL = initialURL;
            mStarFishThread.Start(arg);

            mWaitingForm = new StarFishFontInitWaitForm();
            mWaitingForm.Owner = form;
            mWaitingForm.ShowDialog();
        }

        public void OnCloseWaitingForm(Form form)
        {
            form.Close();
            form.Dispose();
        }

        public void Close()
        {
            PostThreadMessage(mStarFishThreadID, 0x0401, new UIntPtr(), new IntPtr());
        }

        public void Resize(int width, int height)
        {
            PostThreadMessage(mStarFishThreadID, 0x0402, new UIntPtr((uint)width), new IntPtr(height));
        }

        public void Navigate(string url)
        {
            byte[] array = Encoding.UTF8.GetBytes(url);
            IntPtr lpData = Marshal.AllocHGlobal(array.Length);
            Marshal.Copy(array, 0, lpData, array.Length);
            UIntPtr lpLength = new UIntPtr((uint)array.Length);
            PostThreadMessage(mStarFishThreadID, 0x0403, lpLength, lpData);
        }

        private int makeLParam(int LoWord, int HiWord)
        {
            return ((HiWord << 16) | (LoWord & 0xffff));
        }

        private uint makeWParam(int LoWord, int HiWord)
        {
            return (uint)((HiWord << 16) | (LoWord & 0xffff));
        }

        public void dispatchMouseDownEvent(int x, int y)
        {
            int lparam = makeLParam(x, y);
            PostThreadMessage(mStarFishThreadID, 0x0404, new UIntPtr(0), new IntPtr(lparam));
        }

        public void dispatchMouseUpEvent(int x, int y)
        {
            int lparam = makeLParam(x, y);
            PostThreadMessage(mStarFishThreadID, 0x0405, new UIntPtr(0), new IntPtr(lparam));
        }

        public void dispatchMouseMoveEvent(int x, int y, bool isLButtonPressed, bool isRButtonPressed)
        {
            int lparam = makeLParam(x, y);
            uint wparam = makeWParam(isLButtonPressed ? 1: 0, isRButtonPressed ? 1 : 0);
            PostThreadMessage(mStarFishThreadID, 0x0406, new UIntPtr(wparam), new IntPtr(lparam));
        }

        public void dispatchMouseWheelEvent(int x, int y, int z)
        {
            int lparam = makeLParam(x, y);
            PostThreadMessage(mStarFishThreadID, 0x0407, new UIntPtr((uint)z), new IntPtr(lparam));
        }

        public void dispatchKeyDownEvent(Keys keyCode)
        {
            PostThreadMessage(mStarFishThreadID, 0x0413, new UIntPtr(0), new IntPtr((int)keyCode));
        }

        public void dispatchKeyUpEvent(Keys keyCode)
        {
            PostThreadMessage(mStarFishThreadID, 0x0413, new UIntPtr(1), new IntPtr((int)keyCode));
        }

        public void evaluateJS(string js, GotJSResult cb)
        {
            byte[] array = Encoding.UTF8.GetBytes(js);
            IntPtr lpData = Marshal.AllocHGlobal(array.Length);
            Marshal.Copy(array, 0, lpData, array.Length);
            UIntPtr lpLength = new UIntPtr((uint)array.Length);
            lock (mJSResultCallbackList)
            {
                mJSResultCallbackList.Add(cb);
            }
            PostThreadMessage(mStarFishThreadID, 0x0412, lpLength, lpData);
        }

        void lockScreenBitmap()
        {
            mScreenBitmapData = mScreenBitmap.LockBits(new Rectangle(0, 0, mScreenBitmap.Width, mScreenBitmap.Height), ImageLockMode.WriteOnly, mScreenBitmap.PixelFormat);
        }

        private void WorkThread(Object o)
        {
            mStarFishThreadID = GetCurrentThreadId();
            StartArg arg = (StartArg)o;
            int initialWidth = arg.initialWidth;
            int initialHeight = arg.initialHeight;
            mScreenBitmap = new Bitmap(initialWidth, initialHeight, PixelFormat.Format32bppArgb);
            lockScreenBitmap();
            mWebViewInstance = StarFish.createWebViewInstance((uint)initialWidth, (uint)initialHeight, mScreenBitmapData.Scan0, (uint)mScreenBitmapData.Stride);

            {
                string url = arg.initialURL;
                byte[] bytes = Encoding.UTF8.GetBytes(url);
                unsafe
                {
                    fixed (byte* burl = bytes)
                    {
                        loadURL(mWebViewInstance, (IntPtr)burl, (uint)bytes.Length);
                    }
                }

            }

            MSG msg;
            int hasMessage;
            while ((hasMessage = GetMessage(out msg, IntPtr.Zero, 0, 0)) != 0)
            {
                if (hasMessage == -1)
                {
                    break;
                }

                if (msg.message == 0x0401)
                {
                    break;
                }
                else if (msg.message == 0x0402)
                {
                    uint w = (uint)msg.wParam;
                    uint h = (uint)msg.lParam;
                    mScreenBitmap.Dispose();
                    mScreenBitmap = new Bitmap((int)w, (int)h, PixelFormat.Format32bppArgb);
                    lockScreenBitmap();
                    resizeWindow(mWebViewInstance, w, h, mScreenBitmapData.Scan0, (uint)mScreenBitmapData.Stride);
                }
                else if (msg.message == 0x0403)
                {
                    int length = msg.wParam.ToInt32();
                    loadURL(mWebViewInstance, msg.lParam, (uint)length);
                    Marshal.FreeHGlobal(msg.lParam);
                }
                else if (msg.message == 0x0404)
                {
                    int v = msg.lParam.ToInt32();
                    System.Int16 x = BitConverter.ToInt16(BitConverter.GetBytes(v), 0);
                    System.Int16 y = BitConverter.ToInt16(BitConverter.GetBytes(v), 2);
                    dispatchMouseDownEvent(mWebViewInstance, x, y);
                }
                else if (msg.message == 0x0405)
                {
                    int v = msg.lParam.ToInt32();
                    System.Int16 x = BitConverter.ToInt16(BitConverter.GetBytes(v), 0);
                    System.Int16 y = BitConverter.ToInt16(BitConverter.GetBytes(v), 2);
                    dispatchMouseUpEvent(mWebViewInstance, x, y);
                }
                else if (msg.message == 0x0406)
                {
                    int v = msg.lParam.ToInt32();
                    System.Int16 x = BitConverter.ToInt16(BitConverter.GetBytes(v), 0);
                    System.Int16 y = BitConverter.ToInt16(BitConverter.GetBytes(v), 2);
                    int v2 = msg.wParam.ToInt32();
                    System.Int16 l = BitConverter.ToInt16(BitConverter.GetBytes(v), 0);
                    System.Int16 r = BitConverter.ToInt16(BitConverter.GetBytes(v), 2);
                    dispatchMouseMoveEvent(mWebViewInstance, x, y, l == 1 ? true : false, r == 1 ? true : false);
                }
                else if (msg.message == 0x0407)
                {
                    int v = msg.lParam.ToInt32();
                    System.Int16 x = BitConverter.ToInt16(BitConverter.GetBytes(v), 0);
                    System.Int16 y = BitConverter.ToInt16(BitConverter.GetBytes(v), 2);
                    int v2 = msg.wParam.ToInt32();
                    dispatchMouseWheelEvent(mWebViewInstance, x, y, v2);
                }
                else if (msg.message == 0x0408)
                {
                    byte[] buf = new byte[(int)msg.lParam];
                    Marshal.Copy(msg.wParam, buf, 0, buf.Length);
                    string url = Encoding.UTF8.GetString(buf);
                    LocalFree(msg.wParam);

                    try
                    {
                        lock (OnLoadPageStart) { 
                            mForm.Invoke(OnLoadPageStart, new object[] { url });
                        }
                    }
                    catch (Exception e)
                    {
                        System.Diagnostics.Debugger.Log(0, "", e.ToString());
                    }
                }
                else if (msg.message >= 0x0409 && msg.message <= 0x0411)
                {
                    byte[] buf = new byte[(int)msg.lParam];
                    Marshal.Copy(msg.wParam, buf, 0, buf.Length);
                    string txt = Encoding.UTF8.GetString(buf);
                    LocalFree(msg.wParam);

                    MessageKind kind = MessageKind.Info;
                    if (msg.message == 0x0410)
                    {
                        kind = MessageKind.Error;
                    }
                    else if (msg.message == 0x0411)
                    {
                        kind = MessageKind.Warn;
                    }
                    try
                    {
                        lock (OnGotMessage)
                        {
                            mForm.Invoke(OnGotMessage, new object[] { txt, kind });
                        }
                    }
                    catch (Exception e)
                    {
                        System.Diagnostics.Debugger.Log(0, "", e.ToString());
                    }

                }
                else if (msg.message == 0x0412)
                {
                    EvaluateJSResult result = evaluateJS(mWebViewInstance, msg.lParam, (uint)msg.wParam.ToInt32());
                    Marshal.FreeHGlobal(msg.lParam);
                    byte[] buf = new byte[result.len];
                    Marshal.Copy(result.buf, buf, 0, buf.Length);
                    string txt = Encoding.UTF8.GetString(buf);
                    LocalFree(result.buf);

                    lock (mJSResultCallbackList)
                    {
                        GotJSResult cb = (GotJSResult)mJSResultCallbackList[0];
                        mJSResultCallbackList.RemoveAt(0);
                        mForm.Invoke(cb, new object[] { txt });
                    }
                }
                else if (msg.message == 0x0413)
                {
                    int keyCode = msg.lParam.ToInt32();
                    if (msg.wParam.ToInt32() == 1)
                    {
                        dispatchKeyUpEvent(mWebViewInstance, (uint)keyCode);
                    }
                    else
                    {
                        dispatchKeyDownEvent(mWebViewInstance, (uint)keyCode);
                    }
                }
                else
                {
                    giveMessage(mWebViewInstance, msg);
                    if (mLastFrameNumber != drawingBufferFrameNumber(mWebViewInstance))
                    {
                        try
                        {
                            mForm.Invoke(new CloseWaitingForm(OnCloseWaitingForm), new object[] { mWaitingForm });
                        } catch(Exception e)
                        {

                        }
                        
                        mLastFrameNumber = drawingBufferFrameNumber(mWebViewInstance);
                        mScreenBitmap.UnlockBits(mScreenBitmapData);
                        mScreenBitmapData = null;
                        Bitmap newBitmap = new Bitmap(mScreenBitmap.Width, mScreenBitmap.Height, mScreenBitmap.PixelFormat);
                        try
                        {
                            lock (OnScreenBufferUpdate)
                            {
                                mForm.Invoke(OnScreenBufferUpdate, new object[] { mScreenBitmap });
                            }
                        }
                        catch (Exception e)
                        {
                            System.Diagnostics.Debugger.Log(0, "", e.ToString());
                            mScreenBitmap.Dispose();
                        }
                        mScreenBitmap = newBitmap;
                        lockScreenBitmap();
                        updateDrawingBufferAddress(mWebViewInstance, (uint)mScreenBitmap.Width, (uint)mScreenBitmap.Height, mScreenBitmapData.Scan0, (uint)mScreenBitmapData.Stride);
                    }

                }
            }
            
        }

    }
}
