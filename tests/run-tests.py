#!/usr/bin/python3
import os
import sys
import subprocess
import time
import platform


def main():
    cur_folder = ""
    if platform.system() == 'Linux':
        cur_folder = os.getcwd().split("/")[-1]
    else:
        cur_folder = os.getcwd().split("\\")[-1]

    if not cur_folder == "tests":
        print("The script ", sys.argv[0], " hast to be executed under the 'tests' folder")
        exit(1)

    if len(sys.argv) < 2:
        print("You need to supply the absolute path to the jit-testing binary.")
        print("\nUsage:")
        print("\t", sys.argv[0], " /path/to/jit-testing")
        exit(1)

    start_time = time.time()

    JCUT = [sys.argv[1], "test_group.c", "-t", "test-file.txt"]
    if len(sys.argv) > 2:
        i = 2
        while (i < len(sys.argv)):
            JCUT.append(sys.argv[i])
            i += 1

    print(JCUT)

    test_report = []
    test_report_2 = []
    STDOUT_FILE = "stdout.txt"
    STDERR_FILE = "stderr.txt"
    for group in sorted([dir for dir in os.listdir(os.getcwd()) if "group" in dir]):
        os.chdir(group)
        with open(STDOUT_FILE, 'w') as stdout:
            with open(STDERR_FILE, 'w') as stderr:
                ret = subprocess.call(JCUT, stdout=stdout, stderr=stderr)
        test_report.append((ret, group))
        if os.stat(STDOUT_FILE).st_size <= 2:
            os.remove(STDOUT_FILE)
        if os.stat(STDERR_FILE).st_size <= 2:
            os.remove(STDERR_FILE)
        else:
            test_report_2.append(group)
        os.chdir("..")

    # Print test summary
    count = sum([x for x, y in test_report if x < 255])
    if count:
        print("FAILED")
        failed_dir = [dir for ret, dir in test_report if ret == 255]
        if len(failed_dir) > 0:
            print("Failed to run directories: ", failed_dir)
        print("Total number of tests failed: ", count)
        for ret, group in test_report:
            if 0 < ret < 255:
                print("\tDirectory", group, "has", ret, "failed test(s)")
            elif ret < 0 or ret >= 255:
                print("\tJCUT is having problems! debug those issues!")

    # groupH is used to test the error reporting mechanism, that means
    # the test file is plagued with errors.
    if "groupH" in test_report_2:
        test_report_2.remove("groupH")

    if len(test_report_2) > 0:
        print("Tests are failing in", len(test_report_2),"groups.")
        print("---------------------------------------------")
        for group in test_report_2:
            print("Group", group, "has the following errors:")
            os.chdir(group)
            with open(STDERR_FILE, 'r') as stderr:
                print(stderr.read())
            os.chdir("..")
            print("---------------------------------------------")
    else:
        print("SUCCESS!")

    end_time = time.time()
    duration = end_time - start_time
    print('\nFinished in', duration, 'seconds')


if __name__ == "__main__":
    main()
