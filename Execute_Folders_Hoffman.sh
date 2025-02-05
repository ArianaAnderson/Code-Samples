
#!/bin/bash
##Author: Ariana Anderson
## this script runs cry detection on all wav files in subdirectories
#python classify.py --data-dir 009LGH_16_CLEANED --model-dir models/cry_detector --output-dir ./outputs --output-file myout_009LGH_16_CLEANED_cry_detector.csv
#nohup ./Execute_Folders_Hoffman.sh &
#Use on compute node (qrsh)

#!/bin/bash

# Find all subdirectories recursively
find . -maxdepth 1 -type d | while IFS= read -r directory; do
    # Print the current working directory
    echo "Current working directory: $(pwd)"
    # Change into the subdirectory
    cd "$directory" || continue
    # Print the current working directory
    echo "CD- New Current working directory: $(pwd)"
    # Get the name of the subdirectory
    subdirectory=$(basename "$directory")
    
    # Echo the name of the subdirectory
    echo "Subdirectory: $subdirectory"
    # Check if there are .wav files in the directory
    if [ "$(find . -maxdepth 1 -type f -name "*.wav" | wc -l)" -gt 0 ]; then
        # Create a CSV file with the subdirectory name
        csv_file="${subdirectory}_crydetect.csv"
        # Check if classify.py exists in the subdirectory
        if [ ! -f "$csv_file" ]; then

            echo "Processing $directory..."
            python ../classify.py --data-dir . --model-dir ../models/cry_detector --output-dir . --output-file $csv_file

            echo "Output saved to $csv_file"
        fi
    fi
    # Change back to the parent directory
    cd /u/home/a/ariana/project-anderson/Tufts
done
