# Use the lightweight Nginx Alpine image
FROM nginx:alpine

# Remove the default Nginx static assets
RUN rm -rf /usr/share/nginx/html/*

# Copy your index.html into the Nginx html directory
COPY index.html /usr/share/nginx/html/index.html
COPY images/flock.png /usr/share/nginx/html/images/flock.png
COPY images/lilygo.jpg /usr/share/nginx/html/images/lilygo.jpg

# Expose port 80 to the outside world
EXPOSE 80

# Start Nginx and keep it running in the foreground
CMD ["nginx", "-g", "daemon off;"]