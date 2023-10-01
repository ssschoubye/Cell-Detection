mkdir -p output_images
for i in {1..10}; do
  input_filename="samples/hard/${i}HARD.bmp"
  output_filename="output_images/output_image${i}.bmp"
  ./main.exe "$input_filename" "$output_filename"
done