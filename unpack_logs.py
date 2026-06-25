import json
import os
import glob

def main():
    # Find all pretest_logs*.json files in the workspace
    json_files = glob.glob('pretest_logs*.json')
    
    if not json_files:
        print("Error: No pretest_logs*.json files found in the workspace directory!")
        print("Please move the downloaded 'pretest_logs.json' file(s) into this folder and run again.")
        return

    os.makedirs('log_pretest', exist_ok=True)
    total_count = 0

    for json_path in json_files:
        print(f"Reading logs from {json_path}...")
        try:
            with open(json_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
        except Exception as e:
            print(f"Error reading {json_path}: {e}")
            continue

        count = 0
        for battle_id, logs in data.items():
            # Create folder matching the format, e.g., log_pretest/#44909
            folder = os.path.join('log_pretest', f'#{battle_id}')
            os.makedirs(folder, exist_ok=True)
            
            for round_num, log_content in logs.items():
                file_path = os.path.join(folder, f'{round_num}.txt')
                with open(file_path, 'w', encoding='utf-8') as out_f:
                    out_f.write(log_content.strip() + '\n')
                count += 1
        
        print(f"Extracted {count} log files from {json_path}.")
        total_count += count

    print(f"\nSuccessfully extracted total {total_count} log files into log_pretest/ folder!")

if __name__ == '__main__':
    main()
