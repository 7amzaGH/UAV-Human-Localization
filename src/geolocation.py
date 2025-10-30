import math

def get_scale_x(altitude, fov_h, image_width):
    return (altitude * math.tan(math.radians(fov_h / 2))) / (image_width / 2)

def get_scale_y(altitude, fov_v, image_height):
    return (altitude * math.tan(math.radians(fov_v / 2))) / (image_height / 2)

def rotate(dist_x, dist_y, heading):
    theta = math.radians(heading)
    x_prime = math.cos(theta) * dist_x - math.sin(theta) * dist_y
    y_prime = math.sin(theta) * dist_x + math.cos(theta) * dist_y
    return x_prime, y_prime

def get_real_coords(bbox_center, drone_position, drone_heading, altitude, camera_config):
    # Implementation here
    pass
