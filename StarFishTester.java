import com.sun.corba.se.impl.orbutil.concurrent.Mutex;

import java.lang.Exception;
import java.lang.Override;
import java.lang.Runnable;
import java.lang.System;
import java.lang.Thread;
import java.nio.file.*;
import java.io.*;
import java.util.ArrayList;
import java.util.Date;
import java.io.File;
import java.util.Scanner;

import javax.annotation.processing.ProcessingEnvironment;
import javax.sound.midi.SysexMessage;

import sun.security.pkcs11.P11TlsKeyMaterialGenerator;

public class StarFishTester {

    public static class TestResult {
        String testName;
        String status;
    }

	public static void main(String args[]) {
        Boolean nw = false;
		System.out.println("StarFish Tester Start");
		try {
			String testFile="";
			if (args[0].equals("css1")) {
				testFile = "tool/reftest/tclist/wpt_css1.res";
			} else if (args[0].equals("css21")) {
				testFile = "tool/reftest/tclist/wpt_css21.res";
			} else if (args[0].equals("css3_backgrounds")) {
                testFile = "tool/reftest/tclist/wpt_css3_backgrounds.res";
			} else if (args[0].equals("css3_color")) {
                testFile = "tool/reftest/tclist/wpt_css3_color.res";
			} else if (args[0].equals("css3_transforms")) {
                testFile = "tool/reftest/tclist/wpt_css3_transforms.res";
            }

			String fileContent = new String(Files.readAllBytes(Paths.get(testFile)));
			
			String[] files = fileContent.split("\n");
			final ArrayList<String> workQueue = new ArrayList<String>();
			for (int i = 0; i < files.length; i ++) {
				if (files[i].trim().length() != 0) {
					workQueue.add(files[i]);
				}
			}

			int cores = Runtime.getRuntime().availableProcessors();
			// System.out.println("oh! rich guy you have " + cores + " processors!!! good!");
            if (args.length >= 2 && args[1].equals("nw")) {
                nw = true;
                System.out.println("# pixel-test for Node-Webkit");
                /*
                String ss = "";
                ss = "nw tool/pixel_test/nw_capture/ -l " + testFile + " pc";
                System.out.println("# capture START:" + ss);
                Process process = Runtime.getRuntime().exec(ss);
                System.out.println("It may take a few minutes");
                process.waitFor();
                System.out.println("# capture COMPLETE");
                */
            }
			
            final Boolean enableNW = nw;
			final Mutex workQueueMutex = new Mutex();
			final Mutex printMutex = new Mutex();
            final Mutex resultMutex = new Mutex();
            final ArrayList<TestResult> testResult = new ArrayList<TestResult>();


			for (int i = 0; i < workQueue.size(); i ++) {
				TestResult rc = new TestResult();
				rc.testName = workQueue.get(i);
				rc.status = "";
				testResult.add(rc);
			}

			File rm = new File("/tmp/StarFishTester/");
			deleteDirectory(rm);

			final String reftestFolder = "./out/x64/exe/debug/reftest/";
			final String outFolder = "./out/x64/exe/debug/reftest/pixel_test/";

			ArrayList<Thread> threads = new ArrayList<Thread>();
			for (int i = 0; i < cores; i ++) {
				Thread t = new Thread(new Runnable() {
					@Override
					public void run() {
						while(true) {
							String workItem="";
							boolean shouldBreak = false;
							try {
								workQueueMutex.acquire();
							} catch (Exception e) {

							}
							if (workQueue.size() > 0) {
								workItem = workQueue.get(0);
								workQueue.remove(0);
							} else {
								shouldBreak = true;
							}
							workQueueMutex.release();

							if (shouldBreak)
								break;
							Runtime runtime = Runtime.getRuntime();

							// make temp dir

							Date d = new Date();

							String outputString = workItem + " ";

                            // System.out.println(tempDir);

							String caseName = workItem;
                            caseName = caseName.replace("test/reftest", "");
							String folderName = outFolder + caseName;
							folderName = folderName.substring(0, folderName.lastIndexOf('/'));

                            File rDir = new File(folderName);
                            rDir.mkdirs();

							caseName = caseName.substring(0, caseName.lastIndexOf('.'));

                            // excute runner
                            try {

                                String resultFolder = reftestFolder + caseName + "/";
								String resultXML = resultFolder + "/result.xml";
								File xml = new File(resultXML);
								String ss = "";
                                String pngFile = outFolder + caseName + "_result.png";
                                String workingDir = workItem.substring(0, workItem.lastIndexOf('/') + 1);
                                    File png = new File(pngFile);
                                    if (png.exists()) { png.delete(); }
									ss = "./StarFish " + workItem + " --pixel-test --width=" + 800 + " --height=" + 600 + " --screen-shot=" + pngFile;

								// System.out.println(ss);
                                Process process = runtime.exec(ss);
                                process.waitFor();
                            } catch (Exception e) {

                            }

                            // take webkit screen shot
                            String expectedPNG = outFolder + caseName + "_expected.png";
                            if (enableNW)
                                expectedPNG = "./test/reftest/" + caseName.replace("_converted", "_res") + "_expected.png";

							try {

								File sc = new File(expectedPNG);

								if (!sc.exists()) {
                                    if (enableNW) {
                                        System.out.println("File not Found : " + expectedPNG);
                                    }
									String ss = "tool/phantomjs/linux64/bin/phantomjs tool/pixel_test/capture.js -f " + workItem + " " + folderName + " pc";
									// System.out.println(ss);
									Process process = runtime.exec(ss);
									process.waitFor();
								}
							} catch (Exception e) {

							}

							// image diff
							String testStatus="";
							try {
								String ss = "./tool/imgdiff/imgdiff " + outFolder + caseName + "_result.png" + " " + expectedPNG;
								// System.out.println(ss);
								Process process = runtime.exec(ss);
								process.waitFor();
								BufferedReader stdInput = new BufferedReader(new InputStreamReader(process.getInputStream()));

								// read the output from the command
								String s = null;
								String diffOut = "";
								while ((s = stdInput.readLine()) != null) {
									// System.out.println(s);
									diffOut += s;
								}
								if (diffOut.trim().length() > 0) {
									outputString += diffOut;
									testStatus = diffOut;
								} else {
									outputString += "got error";
									testStatus = "diff: 100.0% failed";
								}
								if (testStatus.contains("failed") || testStatus.contains("not exactly same")) {
									ss = "tool/pixel_test/bin/image_diff --diff " + outFolder + caseName + "_result.png" + " " + expectedPNG + " " + outFolder + caseName + "_diff.png";
									process = runtime.exec(ss);
									process.waitFor();
								}
							} catch (Exception e) {

							}

							try {
								printMutex.acquire();
								System.out.println(outputString);
								System.out.flush();
							} catch (Exception e) {
                                System.out.println(e.toString());
							}
							printMutex.release();

							try {
								resultMutex.acquire();
								for (int i = 0; i < testResult.size(); i ++) {
									if (testResult.get(i).testName.equals(workItem)) {
										testResult.get(i).status = testStatus;
										break;
									}
								}
							} catch (Exception e) {
								System.out.println(e.toString());
							}
							resultMutex.release();
						}
					}
				});
				threads.add(t);
                t.start();
			}

			for (int i = 0; i < threads.size(); i++) {
				threads.get(i).join();
			}

			System.err.println("======== test result ==================");
			int good = 0;
			int bad = 0;
			for (int i = 0; i < testResult.size(); i ++) {
				if (testResult.get(i).status.contains("passed"))
					good++;
				else {
					bad++;
				}
				System.err.println(testResult.get(i).testName + " " + testResult.get(i).status);
			}

			System.err.println(" ====total : " + (good + bad) + " passed : " + good + " failed : " + bad);

		} catch(Exception e) {
			System.out.println(e.toString());			
		}



	}

    public static boolean deleteDirectory(File directory) {
        if(directory.exists()){
            File[] files = directory.listFiles();
            if(null!=files){
                for(int i=0; i<files.length; i++) {
                    if(files[i].isDirectory()) {
                        deleteDirectory(files[i]);
                    }
                    else {
                        files[i].delete();
                    }
                }
            }
        }
        return(directory.delete());
    }


}
