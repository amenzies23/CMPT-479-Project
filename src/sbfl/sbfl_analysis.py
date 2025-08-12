import os
import sys
import json
from sbfl.utils import gcov_files_to_frame, get_sbfl_scores_from_frame

if len(sys.argv) != 3:
    print("Usage: python sbfl_analysis.py <gtest_results.json> <coverage_dir>")
    sys.exit(1)

gtest_json = sys.argv[1]
coverage_dir = sys.argv[2]

# parse json tests from the gtest json results
with open(gtest_json, 'r') as f:
    gtest_results = json.load(f)

failing_tests = []
all_tests = []

for suite in gtest_results.get('testsuites', []):
    for case in suite.get('testsuite', []):
        test_id = f"{case.get('classname')}.{case.get('name')}"
        all_tests.append(test_id)
        if 'failures' in case and case['failures']:
            failing_tests.append(test_id)

# print("failing_tests:", failing_tests)
# print("all_tests:", all_tests)

if not failing_tests:
    print("All tests passed. SBFL was not calculated.")
    sys.exit(0)

# build the gcov_files dictionary with test_id as key and a list of gcov files as value
gcov_files = {}
for test_id in all_tests:
    test_path = os.path.join(coverage_dir, test_id)
    if os.path.isdir(test_path):
        gcov_files[test_id] = []
        for file in os.listdir(test_path):
            if file.endswith('.gcov'):
                gcov_files[test_id].append(os.path.join(test_path, file))

cov_df = gcov_files_to_frame(gcov_files, only_covered=True, verbose=False)
# print("cov_df:", cov_df.columns)

# calculate the sbfl scores and save into json
score_df = get_sbfl_scores_from_frame(cov_df, failing_tests=failing_tests)
score_json = score_df.to_json(orient="table")
parsed = json.loads(score_json)
output_path = os.path.join(coverage_dir, 'sbfl_results.json')
with open(output_path, 'w', encoding='utf-8') as f:
    json.dump(parsed, f, ensure_ascii=False, indent=4)
