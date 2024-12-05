

## Experiments

# STEP 1 : DOWNLOAD AND OBSERVE SEGMENTED IMAGES

 - (A) Basmati rice (Original: Rice_basmati.pgm; Segmented: Rice_basmati_seg_bin.pgm),
 - (B) Camargue rice (Original: Rice_camargue.pgm; Segmented: Rice_camargue_seg_bin.pgm),
 - (C) Japanese rice (Original: Rice_japanese.pgm; Segmented: Rice_japanese_seg_bin.pgm)

The rice (A) is Basmati rice, the general shape of the rice grain is that of a long slender cylinder, of ratio 1/5 width to height, in average.

The rice (B) is Camargue rice, the general shape of the rice grain is that of a short cylinder, of ratio 1/3 width to height, in average.

The rice (C) is Japanese rice, the general shape of the rice grain is that of an oval, of ratio 1/2 width to height, in average. This rice seems to be thicker in width as well, relative to the other rices.

 ** We will now treat the .pgm images, whose pixel values are either 0 or 255, for when the pixel is that of a grain of rice or not. **

# STEP 2 : COUNT GRAINS

1. 141 grains of Basmati rice
2. 132 grains of Camargue rice
3. 147 grains of Japanese rice

We can remove the grains that are not fully visible, as they are not fully segmented. We will then count the number of grains that are fully visible.

1. 124 grains of Basmati rice
2. 112 grains of Camargue rice
3. 138 grains of Japanese rice

We can visualize using SVG, the grains of rice that are fully visible.

<!-- svg for a single component -->
![Japanese rice grain 1](resources/Rice_japonais_seg_bin.pgm_component.svg)
 ^ SVG of a single grain of Japanese rice.

--------------------

# STEP 3 : EXTRACT DIGITAL OBJECT BOUNDARY

For each connected component, we extract the inter-pixel boundary.

<!-- svg for a single component, japanese rice -->
![Japanese rice grain 1 boundary](resources/Rice_japonais_seg_bin_boundary.svg)
 ^ SVG of the boundary of a single grain of Japanese rice.

--------------------

# STEP 4 : POLYGONIZE DIGITAL OBJECT BOUNDARY

![Japanese rice grain 1 polygon](resources/Rice_japonais_seg_bin_greedy-dss-decomposition.svg)
 ^ SVG of the polygon of a single grain of Japanese rice.

--------------------

# STEP 5 : CALCUALTE AREA

### Rice Japonais :

Area statistics (Number of 2-cells):

 - Average: 2068.46
 - Median: 2078
 - Minimum: 1744
 - Maximum: 2382

Area statistics (Polygon Area):

 - Average: 8272.46
 - Median: 8322.25
 - Minimum: 6957.5
 - Maximum: 9486

--------------------

### Rice Camargue :

Area statistics (Number of 2-cells):

 - Average: 2693.56
 - Median: 2829
 - Minimum: 921
 - Maximum: 3417

Area statistics (Polygon Area):

 - Average: 10771.7
 - Median: 11292.8
 - Minimum: 3674
 - Maximum: 13708

--------------------

### Rice Basmati :

Area statistics (Number of 2-cells):

 - Average: 2249.97
 - Median: 2321.5
 - Minimum: 801
 - Maximum: 3897

Area statistics (Polygon Area):

 - Average: 8998.58
 - Median: 9314.5
 - Minimum: 3181
 - Maximum: 15559.5

--------------------

## Observations

todo

# STEP 6 : CALCULATE PERIMETER

### Rice Japonais :

Perimeter Statistics (Polygon Perimeter):

 - Average: 441.023
 - Median: 439.707
 - Minimum: 391.414
 - Maximum: 483.414
 - Standard Deviation: 17.5226

Perimeter Statistics (1-Cells Perimeter):

 - Average: 220.362
 - Median: 220
 - Minimum: 196
 - Maximum: 242

--------------------

### Rice Camargue :

Perimeter Statistics (Polygon Perimeter):

 - Average: 544.549
 - Median: 559.414
 - Minimum: 285.414
 - Maximum: 687.414
 - Standard Deviation: 69.4478

Perimeter Statistics (1-Cells Perimeter):

 - Average: 272.107
 - Median: 279
 - Minimum: 142
 - Maximum: 344

--------------------

### Rice Basmati :

 
Perimeter Statistics (Polygon Perimeter):

 - Average: 583.942
 - Median: 591.707
 - Minimum: 304
 - Maximum: 775.414
 - Standard Deviation: 82.1078

Perimeter Statistics (1-Cells Perimeter):

 - Average: 291.952
 - Median: 296
 - Minimum: 152
 - Maximum: 388

--------------------

## Observations

### Distinct Average Perimeters:

Rice Japonica has the smallest average perimeter.
Rice Camargue and Basmati have larger average perimeters.

### Variability:

Rice Japonica shows low variability (SD ≈ 17.5), indicating uniform grain sizes.
Rice Camargue and Basmati exhibit higher variability (SD ≈ 69.4 and 82.1), suggesting a wider range of sizes.

### Overlap in Perimeter Ranges:

Perimeter ranges for Camargue and Basmati overlap significantly.
Japonica's perimeter range is distinct from the others.

### Implications:

Perimeter can be a useful feature for distinguishing Japanese rice from the other two types, though Camargue and Basmati may be harder to differentiate based on this feature alone.

--------------------

## Multigrid take

todo


# STEP 7 : PROPOSE AND CALCULATE CIRCULARITY

# STEP 8 (OPTIONAL): FIND USEFUL MEASURES FOR THE GRAIN CLASSIFICATION

# STEP 9 (OPTIONAL): CLASSIFICATION OF GRAINS

# STEP 10 (OPTIONAL): IMPROVE GRAIN SEGMENTATION