struct OutputBuffer {
  counter: atomic<u32>,
  instance_count: u32,
  first_index: u32,
  base_vertex: u32,
  first_instance: u32,
  v_indices: array<u32>,
};

struct Vertex {
  position: vec4f,
  normal: vec4f
}

struct Time {
  t: f32
};

@group(0)
@binding(0)
var<storage, read_write> internal_indices: OutputBuffer;

@group(0)
@binding(1)
var<storage, read_write> boundary_indices: OutputBuffer;

@group(0)
@binding(2)
var<storage, read_write> vertices: array<Vertex>;

@group(0)
@binding(3)
var<storage, read_write> time: Time;

fn signnz(v: f32) -> f32 { if v >= 0.0 { return 1.0; } else { return -1.0; } }  // sign with signnz(0)=+1

//SDF_FUNCTIONS

//SDF_NORMALS_FUNCTIONS

fn sdf_func(p: vec3<f32>) -> f32 {
  var v = 0.0;
  var normal_id = -1;
//SDF_INVOCATIONS  
  return v;
}

fn sdf_normal_func(p: vec3<f32>, normal_id: i32) -> vec4<f32> {
  var n = vec4(0.0, 0.0, 0.0, 0.0);
//SDF_NORMALS
  return n;
}

@compute
@workgroup_size(1)
fn reset_counter() {
  atomicStore(&internal_indices.counter, 0u);
  atomicStore(&boundary_indices.counter, 0u);
  time.t += 0.01;

  internal_indices.instance_count = 1;
  internal_indices.first_index = 0;
  internal_indices.base_vertex = 0;
  internal_indices.first_instance = 0;

  boundary_indices.instance_count = 1;
  boundary_indices.first_index = 0;
  boundary_indices.base_vertex = 0;
  boundary_indices.first_instance = 0;

}

@compute
@workgroup_size(1)
fn output_vertices(@builtin(global_invocation_id) global_id: vec3<u32>) {
  let x = 2.0 * f32(global_id.x) / f32(1024/16) - 1.0;
  let y = 2.0 * f32(global_id.y) / f32(1024/16) - 1.0;

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


  let index = global_id.y * (1024/16 + 1) + global_id.x;
  let pos = ro + t*rd;
  vertices[index].position = vec4(p.x, p.y, 0.0, 0.0); 
  
  if (t < tmax) {
    vertices[index].position.w = 1.0;
    vertices[index].normal = sdf_normal_func(pos, 0);
  }
}

var<workgroup> local_index_internal : atomic<u32>;
var<workgroup> local_index_boundary : atomic<u32>;

@compute
@workgroup_size(64)
fn output_indices(@builtin(local_invocation_id) lid : vec3<u32>,
        @builtin(global_invocation_id) global_id: vec3<u32>) {

  // Initialize shared counter once per group
  if (lid.x == 0u) {
    atomicStore(&local_index_internal, 0u);
    atomicStore(&local_index_boundary, 0u);
  }
  workgroupBarrier();

  let global_x = global_id.x % (1024/16);
  let global_y = global_id.x / (1024/16);
  
  let i0 = global_y * (1024/16 + 1) + global_x;
  let i1 = (global_y + 1) * (1024/16 + 1) + global_x;
  let i2 = (global_y + 1) * (1024/16 + 1) + (global_x + 1);
  let i3 = global_y * (1024/16 + 1) + (global_x + 1);

  let p0 = vertices[i0].position;
  let p1 = vertices[i1].position;
  let p2 = vertices[i2].position;
  let p3 = vertices[i3].position;

  let output0 = p0.w == 1.0 && p1.w == 1.0 && p2.w == 1.0; 
  let output1 = p0.w == 1.0 && p2.w == 1.0 && p3.w == 1.0; 
  
  let output_boundary0 = !output0 && (p0.w != 0.0 || p1.w != 0.0 || p2.w != 0.0);
  let output_boundary1 = !output1 && (p0.w != 0.0 || p2.w != 0.0 || p3.w != 0.0);

  var add_internal = 0u;
  if (output0) {
    add_internal += 3;
  }
  if (output1) {
    add_internal += 3;
  }

  var add_boundary = 0u;
  if (output_boundary0) {
    add_boundary += 3;
  }
  if (output_boundary1) {
    add_boundary += 3;
  }

  let thread_index_internal = atomicAdd(&local_index_internal, add_internal);
  let thread_index_boundary = atomicAdd(&local_index_boundary, add_boundary);

  workgroupBarrier();
      // One thread per group reserves a block in the global buffer
  var acc_internal: u32 = 0u;
  var acc_boundary: u32 = 0u;
  if (lid.x == 0u) {
      let count_internal = atomicLoad(&local_index_internal);
      acc_internal = atomicAdd(&internal_indices.counter, count_internal);
      // Share base with rest of workgroup
      atomicStore(&local_index_internal, acc_internal);

      let count_boundary = atomicLoad(&local_index_boundary);
      acc_boundary = atomicAdd(&boundary_indices.counter, count_boundary);
      // Share base with rest of workgroup
      atomicStore(&local_index_boundary, acc_boundary);
  }
  workgroupBarrier();
  // Read the base (now stored back into localCounter)
  let base_internal = atomicLoad(&local_index_internal);
  let base_boundary = atomicLoad(&local_index_boundary);
   

  let internal_global_index = base_internal + thread_index_internal;
  let boundary_global_index = base_boundary + thread_index_boundary;

  add_internal = 0u;
  add_boundary = 0u;
  if (output0) {
    internal_indices.v_indices[internal_global_index + add_internal] = i0;
    add_internal++;
    internal_indices.v_indices[internal_global_index + add_internal] = i1;
    add_internal++;
    internal_indices.v_indices[internal_global_index + add_internal] = i2;
    add_internal++;
  } else if (output_boundary0) {
    boundary_indices.v_indices[boundary_global_index + add_boundary] = i0;
    add_boundary++;
    boundary_indices.v_indices[boundary_global_index + add_boundary] = i1;
    add_boundary++;
    boundary_indices.v_indices[boundary_global_index + add_boundary] = i2;
    add_boundary++;

  }
  if (output1) {
    internal_indices.v_indices[internal_global_index + add_internal] = i0;
    add_internal++;
    internal_indices.v_indices[internal_global_index + add_internal] = i2;
    add_internal++;
    internal_indices.v_indices[internal_global_index + add_internal] = i3;
    add_internal++;
  } else if (output_boundary1) {
    boundary_indices.v_indices[boundary_global_index + add_boundary] = i0;
    add_boundary++;
    boundary_indices.v_indices[boundary_global_index + add_boundary] = i2;
    add_boundary++;
    boundary_indices.v_indices[boundary_global_index + add_boundary] = i3;
    add_boundary++;
  }
}


