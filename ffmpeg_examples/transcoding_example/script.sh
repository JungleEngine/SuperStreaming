#!/bin/sh

if [ -d "crfFinder" ]; then
	rm -r crfFinder
fi
mkdir crfFinder
if [ "$1" != "" ]; then
	original=$1
else
	echo "No input file specified!"
	exit 1
fi
echo $original

ffmpeg -hide_banner -loglevel panic -ss 0 -i $original -t 5 -c copy crfFinder/original_sample.mp4
cd crfFinder
original_sample_size_kb=`du -k original_sample.mp4 | cut -f1`

optimal_crf=18
for crf in $(seq 18 50);do
	ffmpeg -hide_banner -loglevel panic -i original_sample.mp4 -c:v libx264 -crf $crf -preset slow -c:a copy out$crf.mp4
	file_size_kb=`du -k out$crf.mp4 | cut -f1`
	echo "crf: " $crf " , file size: " $file_size_kb
	if [ $file_size_kb -le $original_sample_size_kb ]; then
		optimal_crf=$crf
		break
	fi 
done

global_optimal_crf=$optimal_crf
original_full_size_kb=`du -k ../$original | cut -f1`

for crf in $(seq $optimal_crf 30); do
	echo "trying the sample optimal crf on the original video"
	ffmpeg -hide_banner -loglevel panic -i ../$original -c:v libx264 -crf $crf -preset slow -c:a copy fullVideo$crf.mp4
	file_size_kb=`du -k fullVideo$crf.mp4 | cut -f1`
	echo "original video size is: $original_full_size_kb"
	echo "converted video size is: $file_size_kb"
	if [ $file_size_kb -le $original_full_size_kb ]; then
		global_optimal_crf=$crf
		break
	fi
	diff=$((file_size_kb - original_full_size_kb))
	perc=$(((100*diff)/original_full_size_kb))
	echo $perc
	if  [ $perc -le 2 ] ; then		
		global_optimal_crf=$crf
		echo "difference between original and re-encoded file is less than 2% originl: $original_full_size_kb re-encoded: $file_size_kb"
		echo "global optimal crf is : $global_optimal_crf"
		break
	fi
done 
cd ..
echo $global_optimal_crf > ${original%.*}.meta
rm -r crfFinder
