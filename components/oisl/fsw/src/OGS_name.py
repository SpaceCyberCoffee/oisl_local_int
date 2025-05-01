import sys
import json

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python extract_ogs_name.py <file_path>")
        sys.exit(1)

    file_path = sys.argv[1]
    # print("File Path:",file_path)
    with open(file_path, 'r') as f:
        # Always provide the OGS NAME as Ilgrim by default for large file transfer. 
        try: 
            file_content = f.read()
            # print("File content received:\n", file_content)

            # Parse JSON content
            data = json.loads(file_content)
            ogs_name = data['instructions']['DL']['ground_station_name']
            print(f"OGS Name: {ogs_name}")
        except:
            print(f"OGS Name: Igrim")
    
