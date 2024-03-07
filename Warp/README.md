## Warp tool 
### Introduction.
YoloV4tinyOCR4 uses a bird's view to track objects. This tool helps you to find the best settings.<br><br>
To improve the outcomes, the scene is manipulated as been seen from above.<br>
There are several techniques possible. Here, the simplest one is used; an exponential distortion of the vertical axis.<br>
It is a quick method with good results. Remember, we only project an object's (x,y) position in this domain.<br>
It does not have the pretensions to give an actual bird's view representation of camera images.<br><br>
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/24c0d155-7836-46ae-8ff2-899a5570bb69)<br><br>

## Use.
The usage is simple. Start the tool in a terminal and select a picture.<br>
With the two scrollbars, you can manipulate the image till it gives a satisfactory result.<br>
Note the 'warp' and 'gain' results shown in the terminal. These values are requested in the settings.<br>
Every camera has its unique position, hence its own warp and gain signature.<br><br>
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/13b2af05-787b-4fc7-ba22-1dd423c3a3a7)
> Warp = 1.0    Gain = 1.0<br>

![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/d2b2a5e1-196a-4c73-a120-eed3ca40196d)
> Warp = 4.4    Gain = 1.5<br>

As you can see, the warp and gain numbers don't match with the one above the scrollbars.<br>
The scrollbars are rudimentary OpenCV GUI elements, not capable of adjusting floats. You must use the numbers in the terminal.<br>
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/905a44d1-0474-4e07-83f6-cbb7ea893079)<br>

![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/ae30ae4c-9780-4d70-a283-d2522dd4930b)
> Warp = 1.0    Gain = 1.0<br>
 
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/0670aa10-7a26-423c-ab5a-24839fbdd175)
> Warp = 4.4    Gain = 1.5<br>


