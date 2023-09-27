mkdir -p output_images
for i in {1..10}; do
  input_filename="samples/easy/${i}EASY.bmp"
  output_filename="output_images/output_image${i}.bmp"
  ./main.exe "$input_filename" "$output_filename"
done
