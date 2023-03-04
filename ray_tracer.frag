#version 150

#define NUM_SPHERES 25
#define NUM_LIGHT_SOURCES 2
#define MAX_DEPTH 2
#define RAY_ARRAY_LENGTH int(pow(2, MAX_DEPTH+1)) + 1
#define M_PI radians(180.0f)
#define GLASS_ETA 1.52f
#define MAX_PORTAL_DEPTH 10

// Have pixel coordinates not be in the center of the pixels, used for AA calculations
layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform vec2 resolution;
uniform float aspect_ratio;
uniform float fov;
uniform float time;
uniform mat4 inverse_view_matrix;
uniform int aa = 2;

// Spheres
uniform mat3 rot[NUM_SPHERES];
uniform vec3 pos[NUM_SPHERES];
uniform vec3 posFake[NUM_SPHERES];
uniform vec3 color[NUM_SPHERES];
uniform vec2 k_rt[NUM_SPHERES];
uniform float radius[NUM_SPHERES];
// Portals
uniform int spherePortalIndex[NUM_SPHERES];
uniform int insidePortal[NUM_SPHERES];
// Textures
uniform int textureIndex[NUM_SPHERES];
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;

// Lighting
uniform vec3 light_source_pos_arr[NUM_LIGHT_SOURCES];
uniform float light_source_intensity[NUM_LIGHT_SOURCES];
uniform float specular_exponent[NUM_LIGHT_SOURCES];

struct Ray
{
    vec3 origin;
    vec3 direction;
    float pass_through_rate;
    bool valid;
};
Ray rays[RAY_ARRAY_LENGTH];

struct HitObject
{
    int index;
    vec3 hitPos;
    vec3 hitPosFar;
    float intensity;
    mat3 rotation;
    vec3 position;
    vec3 color;
    vec2 k_rt;
    float radius;
    int spherePortalIndex;
    int textureIndex;
};

out vec4 out_Color;

HitObject findIntersection(vec3 origin, vec3 direction, bool ignorePortals)
{
    int closest_object_index = -1;
    float closest_object_dist_near = -1.0f;
    float closest_object_dist_far = -1.0f;
    for (int i = 0; i < NUM_SPHERES * 2; ++i)
    {
        vec3 a;
        int index = i;
        if (i >= NUM_SPHERES && insidePortal[i - NUM_SPHERES] == -1)
            continue;
        else if (i >= NUM_SPHERES)
        {
            a = posFake[i - NUM_SPHERES] - origin;
            index = i - NUM_SPHERES;
        }
        else
            a = pos[i] - origin;

        float discriminant = pow(dot(a, direction), 2) - pow(length(a), 2) + pow(radius[index], 2);

        if (discriminant >= 0)
        {
            float d1 = dot(a, direction) + sqrt(discriminant);
            float d2 = dot(a, direction) - sqrt(discriminant);
            if (d1 > 0 && d2 > 0)
            {
                // Ignore portals if set to true, so they don't cast shadows for example
                if (ignorePortals && spherePortalIndex[index] != -1)
                    continue;

                // Make sure that shadows are cut off when spheres enters portals
                if (ignorePortals && insidePortal[index] != -1)
                {
                    vec3 hit_position = origin + min(d1, d2) * direction;
                    if ((i >= NUM_SPHERES
                         && length(hit_position - pos[spherePortalIndex[insidePortal[index]]])
                             <= radius[spherePortalIndex[insidePortal[index]]])
                         || (i < NUM_SPHERES && length(hit_position - pos[insidePortal[index]])
                             <= radius[insidePortal[index]]))
                        continue;
                }

                float dist = min(d1, d2);
                if (dist < closest_object_dist_near || closest_object_dist_near < 0)
                {
                    closest_object_index = i;
                    closest_object_dist_near = min(d1, d2);
                    closest_object_dist_far = max(d1, d2);
                }
            }
        }
    }

    vec3 position;
    if (closest_object_index >= NUM_SPHERES)
    {
        closest_object_index = closest_object_index - NUM_SPHERES;
        position = posFake[closest_object_index];
    }
    else
        position = pos[closest_object_index];

    return HitObject(closest_object_index,
                     origin + (direction*closest_object_dist_near),
                     origin + (direction*closest_object_dist_far),
                     0.0f,
                     rot[closest_object_index],
                     position,
                     color[closest_object_index],
                     k_rt[closest_object_index],
                     radius[closest_object_index],
                     spherePortalIndex[closest_object_index],
                     textureIndex[closest_object_index]);
}

float calculateADSLighting(HitObject obj, vec3 direction, vec3 point_normal)
{
    float local_intensity = 0.0f;

    for (int i = 0; i < NUM_LIGHT_SOURCES; ++i)
    {
        vec3 point_to_light = light_source_pos_arr[i] - obj.hitPos;
        vec3 point_to_light_dir = normalize(point_to_light);

        // Move ray origin slightly outwards, to help with intersection checks
        HitObject otherObj = findIntersection(obj.hitPos + point_to_light_dir * 0.01f, point_to_light_dir, true);

        // If there are no spheres between our point and the light, calculate light intensity
        if (otherObj.index == -1 || length(point_to_light) < length(otherObj.hitPos - obj.hitPos))
        {
            vec3 mirrored = reflect(-point_to_light_dir, point_normal);
            float i_diff = max(0.0f, dot(point_to_light_dir, point_normal));
            float i_spec = pow(max(0.0f, dot(normalize(mirrored), -direction)), specular_exponent[i]);
            local_intensity += light_source_intensity[i] * (i_diff + i_spec);
        }
    }
    return local_intensity;
}

float nonPeriodicFunction(float x)
{
    return sin(2.0f * x) + sin(M_PI * x) + 2.0f;
}

HitObject castRay(vec3 origin, vec3 direction)
{
    HitObject obj = findIntersection(origin, direction, false);

    // Check for portal intersection and send it through if the ray hit a portal
    int count = 0;
    while (obj.index != -1 && obj.spherePortalIndex != -1 && count < MAX_PORTAL_DEPTH)
    {
        ++count;
        // Calculate the point normal for the point on the other side of the connected portal
        vec3 point_normal = obj.hitPos - obj.position;
        vec3 originToSphereDir = normalize(obj.position - origin);
        // Multiply by 2.01 radii, to get the position slightly outside, to help with intersection checks
        float throughDistance = abs(dot(normalize(point_normal), originToSphereDir)) * 2.01f * radius[obj.spherePortalIndex];
        vec3 point_normal_far = normalize(point_normal) * radius[obj.spherePortalIndex] + originToSphereDir * throughDistance;

        // Make edges of portal prettier by adding a wave effect
        float edgeProximity = abs(dot(direction, normalize(point_normal)));
        float waterEffect = 0.02f * nonPeriodicFunction(time/300.0f + 180.0f / M_PI * edgeProximity) * pow(1.0f - edgeProximity, 3);
        float eta = 1.0f + pow(1.0f - edgeProximity - waterEffect, 7);
        direction = refract(direction, normalize(point_normal), 1.0f / eta);

        // Update ray origin
        origin = pos[obj.spherePortalIndex] + point_normal_far + direction*0.01f;
        obj = findIntersection(origin, direction, false);
    }
    if (count == MAX_PORTAL_DEPTH)
        obj.index = -1;

    // Calculate light for the point on the potentially hit object
    if (obj.index != -1)
    {
        vec3 point_normal = normalize(obj.hitPos - obj.position);
        obj.intensity = calculateADSLighting(obj, direction, point_normal);
    }
    return obj;
}

vec3 render(vec3 origin, vec3 direction)
{
    // Initialize rays array to empty rays to avoid junk values and warnings
    for (int i = 0; i < RAY_ARRAY_LENGTH; ++i)
        rays[i] = Ray(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f, false);

    vec3 rgb = vec3(0.0f, 0.0f, 0.0f);
    rays[0] = Ray(origin, direction, 1.0f, true);

    for (int i = 0; i <= MAX_DEPTH; ++i)
    {
        for (int j = 0; j < int(pow(2, i)); ++j)
        {
            Ray r = rays[j + int(pow(2, i)) - 1];

            if (r.valid)
            {
                // Cast ray to first object
                HitObject obj = castRay(r.origin, r.direction);
                if (obj.index != -1)
                {
                    // Mix color or texture
                    if (obj.textureIndex != -1)
                    {
                        // Calculate texture coordinates
                        vec3 n = obj.hitPos - obj.position;
                        n = vec3(obj.rotation * n);
                        float theta = atan(-n.z, n.x);
                        float u = (theta + M_PI) / (2.0f * M_PI);
                        float phi = acos(-n.y / obj.radius);
                        float v = phi / (2.0f * M_PI);

                        // Scale texture
                        u *= max(2.0f, obj.radius);
                        v *= max(2.0f, obj.radius);

                        // Dynamic indexing not supported with sampler2D, therefore using this
                        // TODO: Array textures.
                        if (obj.textureIndex == 0)
                            rgb = mix(rgb, vec3(texture(tex0, vec2(u, v))), obj.intensity * r.pass_through_rate);
                        else if (obj.textureIndex == 1)
                            rgb = mix(rgb, vec3(texture(tex1, vec2(u, v))), obj.intensity * r.pass_through_rate);
                        else if (obj.textureIndex == 2)
                            rgb = mix(rgb, vec3(texture(tex2, vec2(u, v))), obj.intensity * r.pass_through_rate);
                        else
                            rgb = mix(rgb, vec3(texture(tex3, vec2(u, v))), obj.intensity * r.pass_through_rate);
                    }
                    else
                    {
                        rgb = mix(rgb, obj.color, obj.intensity * r.pass_through_rate);
                    }

                    // If object has reflective properties
                    if (obj.k_rt.x > 0.0f)
                    {
                        vec3 point_normal = normalize(obj.hitPos - obj.position);
                        vec3 reflected_ray = reflect(r.direction, point_normal);

                        // Move the ray slightly out from the surface when saving it to help with intersection
                        rays[j*int(pow(2, i)) + int(pow(2, i+1)) - 1] = Ray(obj.hitPos + reflected_ray*0.01f, reflected_ray,
                                                                            obj.k_rt.x * r.pass_through_rate,
                                                                            true);
                    }
                    // If object has refractive properties
                    if (obj.k_rt.y > 0.0f)
                    {
                        // Calculate the refracted in-ray
                        float eta = GLASS_ETA;
                        vec3 point_normal = normalize(obj.hitPos - obj.position);
                        vec3 refracted_ray = refract(r.direction, point_normal, 1.0f / eta);

                        // Find the position this ray intersects with on the other side of the sphere
                        obj = findIntersection(obj.hitPos - refracted_ray*0.01f, refracted_ray, false);
                        vec3 point_normal_far = normalize(obj.hitPosFar - obj.position);
                        // Refract it out of the sphere. Refract needs the vector that goes meets the surface from outside
                        // We save the negative result, to get the vector that continues out from the sphere
                        refracted_ray = -refract(-refracted_ray, point_normal_far, eta);

                        // Move the ray slightly out from the surface when saving it to help with intersection
                        if (obj.index != -1)
                            rays[j*int(pow(2, i)) + int(pow(2, i+1))] = Ray(obj.hitPosFar + refracted_ray*0.01f,
                                                                            refracted_ray,
                                                                            obj.k_rt.y * r.pass_through_rate,
                                                                            true);
                    }
                }
            }
        }
    }

    return rgb;
}

vec3 calculateDirectionFromPixel(float x_p, float y_p, vec3 camera_pos)
{
    // Calculate the camera coordinates
    float x = ((2.0f * x_p / resolution.x) - 1.0f) * aspect_ratio * fov;
    float y = ((2.0f * y_p / resolution.y) - 1.0f) * fov;

    // Calculate the starting position and the direction in world coordinates
    vec3 point = vec3(inverse_view_matrix * vec4(x, y, -1.0f, 1.0f));
    vec3 direction = normalize(point - camera_pos);
    return direction;
}

vec3 renderWithAA(vec3 camera_pos, int x_aa)
{
    vec3 rgb = vec3(0.0f, 0.0f, 0.0f);
    float x2, y2, step_size = 1.0f / x_aa;

    // Send rays in a pattern within the pixel
    for (int y = 0; y < x_aa; ++y)
    {
        y2 = step_size / 2.0f + y * step_size;
        for (int x = 0; x < x_aa; ++x)
        {
            x2 = step_size / 2.0f + x * step_size;

            vec3 direction = calculateDirectionFromPixel(gl_FragCoord.x + x2, gl_FragCoord.y + y2, camera_pos);
            rgb += render(camera_pos, direction);
        }
    }

    // Divide by amount of rays
    return rgb / pow(x_aa, 2);
}

void main(void)
{
    // Calculate camera coordinates
    vec4 origin = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vec3 camera_pos = vec3(inverse_view_matrix * origin);

    // Render with optional supersampling
    vec3 rgb = renderWithAA(camera_pos, aa);
    out_Color = vec4(rgb, 0.0f);
}
