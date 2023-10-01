mkdir -p output_images
for i in {1..5}; do
  input_filename="samples/impossible/${i}IMPOSSIBLE.bmp"
  output_filename="output_images/output_image${i}.bmp"
  ./main.exe "$input_filename" "$output_filename"
done