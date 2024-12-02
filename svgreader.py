import xml.etree.ElementTree as ET

def filter_grid_elements(input_file, output_file):
    # Parse the SVG file
    tree = ET.parse(input_file)
    root = tree.getroot()

    # Namespace used in SVG files
    namespace = {"svg": "http://www.w3.org/2000/svg"}
    
    # Counters for removed elements
    circles_removed = 0
    lines_removed = 0

    # Remove gray grid lines
    for line in root.findall(".//svg:line", namespace):
        stroke = line.attrib.get("stroke", "")
        stroke_width = line.attrib.get("stroke-width", "")
        
        # Identify grid lines (gray with specific stroke-width)
        if stroke == "rgb(160,160,160)" and stroke_width == "0.353mm":
            root.remove(line)
            lines_removed += 1
            print(lines_removed)

    # Remove grid-style circles
    for circle in root.findall(".//svg:circle", namespace):
        fill = circle.attrib.get("fill", "")
        radius = circle.attrib.get("r", "")
        
        # Identify grid dots (gray circles with specific radius)
        if fill == "rgb(160,160,160)" and radius == "2.83":
            root.remove(circle)
            circles_removed += 1
            print(circles_removed)

    # Output results
    print(f"Removed {lines_removed} grid lines.")
    print(f"Removed {circles_removed} grid dot circles.")
    
    # Save the filtered SVG
    tree.write(output_file, encoding="utf-8", xml_declaration=True)
    print(f"Filtered SVG saved to: {output_file}")

# Example usage
input_svg = "resources/Rice_japonais_seg_bin_freeman_chain.svg"
output_svg = "resources/Rice_japonais_seg_bin_freeman_chain_TEST.svg"
filter_grid_elements(input_svg, output_svg)