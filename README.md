# starbound
Utility for counting stars in the sky.  

Usage:  
starbound.exe *file_name* *intensity_threshold* *star_size_min* 

*file_name* - path to .bmp picture of the sky  
*intensity_threshold* - minimum brightness of pixel that counts as star pixel. Can take absolute value in range [0.000...1.000] or value relative to the average intensity of the picture. In this case, "x" should be added, e.g. 8.0x.
*star_size_min* - minimum star size in pixels

Example usage:  
starbound.exe sky.bmp 3.5x 5 - relative intensity 3.5x, min size 5px  
starbound.exe sky.bmp 0.15 12 - absolute intensity 0.15, min size 12px  

Output:  
	1) Text report with filename *file_name*_report.txt  
	2) JSON-formatted report with filename *file_name*_report.json  
	3) BMP image with filename *file_name*_report.bmp  


