import os
import sys
import pandas as pd
from sbfl.utils import gcov_files_to_frame, get_sbfl_scores_from_frame
import json

def main():
    FAILED_TESTS_FILE = sys.argv[1]
    COVERAGE_BASE_DIR = sys.argv[2] 

    print(f"Using failed tests file: {FAILED_TESTS_FILE}")
    print(f"Using coverage directory: {COVERAGE_BASE_DIR}")

    # format of .log file:
    # 2:positive
    def get_failing_tests_from_ctest():
        failing_tests = []
        
        if os.path.exists(FAILED_TESTS_FILE):
            with open(FAILED_TESTS_FILE, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line:
                        if ':' in line:
                            test_name = line.split(':', 1)[1]
                            failing_tests.append(f"test_{test_name}")
        return failing_tests


    # get all the 
    def find_test_coverage_directories():
        gcov_dir = {}
        
        if not os.path.exists(COVERAGE_BASE_DIR):
            return gcov_dir
        
        for item in os.listdir(COVERAGE_BASE_DIR):
            item_path = os.path.join(COVERAGE_BASE_DIR, item)
            
            if os.path.isdir(item_path):
                gcov_dir[item] = item_path 
        return gcov_dir

    gcov_dir = find_test_coverage_directories()

    gcov_files = {test:[] for test in gcov_dir}
    for test in gcov_dir:
        for path in os.listdir(gcov_dir[test]):
            if path.endswith('.gcov'):
                gcov_files[test].append(os.path.join(gcov_dir[test], path))
        print(f"{test}: {len(gcov_files[test])} gcov files are found.")

    cov_df = gcov_files_to_frame(gcov_files, only_covered=True, verbose=False)

    failing_tests = get_failing_tests_from_ctest()

    if failing_tests:
        score_df = get_sbfl_scores_from_frame(cov_df, failing_tests=failing_tests)
        score_df = score_df.to_json(orient="table")
        parsed = json.loads(score_df)
        with open('data.json', 'w', encoding='utf-8') as f:
            json.dump(parsed, f, ensure_ascii=False, indent=4)
    else:
        print("all tests passed. SBFL was not calculated")


