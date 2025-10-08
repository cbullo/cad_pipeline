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
 
  let light = dot(normal, surfaceToLightDirection);
 
  // Lets multiply just the color portion (not the alpha)
  // by the light
  let color = vec3(1.0, 0.0, 0.0) * light;
  return vec4f(color, 1.0);
}

fn signnz(v: f32) -> f32 { if v >= 0.0 { return 1.0; } else { return -1.0; } }  // sign with signnz(0)=+1

fn sdBox( p: vec3<f32>, b: vec3<f32> ) -> f32 {
  let q: vec3<f32> = abs(p) - b;
  return length(max(q,vec3<f32>(0.0))) + min(max(q.x,max(q.y,q.z)),0.0);
};

fn sdBoxNormal( p: vec3f, b: vec3f ) -> vec4f {
  let slack = b - abs(p);                   // ~0 on the touching face(s)
  // Pick the face with minimal slack (ties -> consistent pick).
  if (slack.x <= slack.y && slack.x <= slack.z) {
      return vec4f(signnz(p.x), 0.0, 0.0, 0.0);
  } else if (slack.y <= slack.z) {
      return vec4f(0.0, signnz(p.y), 0.0, 0.0);
  } else {
      return vec4f(0.0, 0.0, signnz(p.z), 0.0);
  }
};



fn sdf_func(p: vec3<f32>) -> f32 {
  var v = 0.0;
  var normal_id = -1;
  v += sdBox(p, vec3<f32>(0.3, 0.3, 0.3));
  return v;
}

fn sdf_normal_func(p: vec3<f32>, normal_id: i32) -> vec4<f32> {
  var n = vec4(0.0, 0.0, 0.0, 0.0);
  n = sdBoxNormal(p, vec3<f32>(0.3, 0.3, 0.3));
  return n;
}


@vertex
fn vs_boundary_main(vertex: VertexInput) -> VertexOutput {
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
fn fs_boundary_main(vsOut: VertexOutput) -> @location(0) vec4f {
  let x = 2.0 * f32(vsOut.position.x) / f32(1024) - 1.0;
  let y = 1.0 - 2.0 * f32(vsOut.position.y) / f32(1024);

// camera movement	
	let an: f32 = 0.3;
	let ro: vec3<f32> = vec3<f32>( 1.0*cos(an), 1.5 * sin(an), 1.0*sin(an) );
  let ta: vec3<f32> = vec3<f32>( 0.0, 0.0, 0.0 );
  // camera matrix
  let ww: vec3<f32> = normalize( ta - ro );
  let uu: vec3<f32> = normalize( cross(ww,vec3<f32>(0.0,1.0,0.0) ) );
  let vv: vec3<f32> = normalize( cross(uu,ww));

  var tot: vec3<f32> = vec3<f32>(0.0);
  
  let p = vec2<f32>(x, y);

  // create view ray
  let rd = normalize( p.x*uu + p.y*vv + 1.5*ww );

  // raymarch
  const tmax: f32 = 3.0;
  var t: f32 = 0.0;
  var n_id: i32 = -1;
  for (var i = 0; i < 256; i=i+1)
  {
      let pos = ro + t*rd;
      let h = sdf_func(pos);
      if (h<0.0001 || t>tmax) { 
        break; 
      }
      t += h;
  }

  if t >= tmax {
    return vec4(0.0, 0.0, 0.0, 0.0);
  }

  let pos = ro + t * rd;
  let normal = sdf_normal_func(pos, 0).xyz;

  // Because vsOut.normal is an inter-stage variable 
  // it's interpolated so it will not be a unit vector.
  // Normalizing it will make it a unit vector again
  let surfaceToLightDirection = normalize(vsOut.surfaceToLight);  
 
  let light = dot(normal, surfaceToLightDirection);
 
  // Lets multiply just the color portion (not the alpha)
  // by the light
  let color = vec3(1.0, 0.0, 0.0) * light;
  return vec4f(color, 1.0);
}