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
		System.out.println("StarFish Tester Start");
		try {
			String testFile="";
			if (args[0].equals("css1")) {
				testFile = "tool/pixel_test/css1.res";
			} else if (args[0].equals("css21")) {
				testFile = "tool/pixel_test/css21.res";
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

			final String outFolder = "./out/x64/exe/debug/reftest/pixel_test/";

			File rDir = new File(outFolder + "test/reftest/csswg-test/css1/");
			rDir.mkdirs();

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

							String tempDir = "/tmp/StarFishTester/" + d.getTime() + (int)(Math.random()*1000) + "/";


							String outputString = workItem + " ";

                            // System.out.println(tempDir);
                            File desti = new File(tempDir);
                            desti.mkdirs();

							String folderName = outFolder + workItem;
							folderName = folderName.substring(0, folderName.lastIndexOf('/'));

							String caseName = workItem;
							caseName = caseName.substring(0, caseName.indexOf('.'));

                            // excute runner
                            try {
                                String ss = "./run.sh " + workItem + " --result-folder="  + tempDir + " --pixel-test --width=" + 800 + " --height=" + 600 + " --screen-shot=" + outFolder + caseName + "_result.png";
								// System.out.println(ss);
                                Process process = runtime.exec(ss);
                                process.waitFor();
                            } catch (Exception e) {

                            }

                            // take webkit screen shot
							try {
								String ss = "tool/phantomjs/linux64/bin/phantomjs tool/pixel_test/capture.js -f " + workItem + " " + folderName + " pc";
								// System.out.println(ss);
								Process process = runtime.exec(ss);
								process.waitFor();
							} catch (Exception e) {

							}

							// image diff
							String testStatus="";
							try {
								String ss = "tool/pixel_test/bin/image_diff " + outFolder + caseName + "_result.png" + " " + outFolder + caseName + "_expected.png";
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
								if (testStatus.contains("failed")) {
									ss = "tool/pixel_test/bin/image_diff --diff " + outFolder + caseName + "_result.png" + " " + outFolder + caseName + "_expected.png " + outFolder + caseName + "_diff.png";
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
			int check = 0;
			int bad = 0;
			for (int i = 0; i < testResult.size(); i ++) {
				if (testResult.get(i).status.contains("passed"))
					good++;
				else {
					String in = testResult.get(i).status;
					in = in.substring(in.indexOf(' '), in.length());
					// System.out.println(in);
					in = in.substring(0, in.indexOf('%'));
					// System.out.println(in);

					float f = Float.parseFloat(in);
					if (f < 1.0)
						check++;
					else
						bad++;
				}
				System.err.println(testResult.get(i).testName + " " + testResult.get(i).status);
			}

			System.err.println("total : " + (good + check + bad) + " passed : " + good + " check : " + check + " failed : " + bad);

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
