# starbound
Utility for counting stars in the sky.  

Usage:  
starbound.exe *file_name* [intensity_threshold]  

*file_name* - path to .bmp picture of the sky  
[intensity_threshold] - optional. Minimum brightness of pixel that counts as star pixel. Can take absolute value in range [0.000...1.000] or value relative to the average intensity of the picture. In this case, "x" should be added, e.g. 8.0x. By default, this option have a value of 2.0x.
 
Example usage:  
starbound.exe sky.bmp 3.5x - relative intensity  
starbound.exe sky.bmp 0.15 - absolute intensity  

Output:  
	1) Text report with filename *file_name*_report.txt  
	2) BMP image with filename *file_name*_report.bmp  


