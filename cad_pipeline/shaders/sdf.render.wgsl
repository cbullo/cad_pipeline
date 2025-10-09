struct VertexInput {
    @location(0) position: vec4<f32>,
    @location(1) normal: vec4<f32>
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) normal: vec4f,
    @location(1) surfaceToLight: vec3f
}

@vertex
fn vs_main(vertex: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(vertex.position.xyz, 1);
    out.normal = vertex.normal;

    // Compute the world position of the surface
    let surfaceWorldPosition = vertex.position.xyz;
    
    // Compute the vector of the surface to the light
    // and pass it to the fragment shader
    out.surfaceToLight = vec3f(2.f, 2.f, 1.f) - surfaceWorldPosition;

    return out;
}

@fragment
fn fs_main(vsOut: VertexOutput) -> @location(0) vec4f {
  // Because vsOut.normal is an inter-stage variable 
  // it's interpolated so it will not be a unit vector.
  // Normalizing it will make it a unit vector again
  let normal = normalize(vsOut.normal.xyz);
  let surfaceToLightDirection = normalize(vsOut.surfaceToLight);

  let light = max(dot(normal, surfaceToLightDirection), 0.0);

  // Lets multiply just the color portion (not the alpha)
  // by the light
  let color = vec3(0.1, 0.0, 0.0) + vec3(0.9, 0.0, 0.0) * light;
  return vec4f(color, 1.0);
}
