/**
 * This file is part of the nestk library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Nicolas Burrus <nicolas.burrus@uc3m.es>, (C) 2010
 */

#include "mesh.h"
#include "ply.h"

#include <ntk/utils/debug.h>
#include <ntk/utils/opencv_utils.h>
#include <ntk/numeric/utils.h>

// #include <opencv/cv.h>

#include <ntk/utils/time.h>

#include <fstream>
#include <cstring>
#include <errno.h>

using namespace cv;

namespace
{

struct PlyVertex
{
    float x,y,z;
    float nx,ny,nz;
    float u,v; // tex coords.
    unsigned char r,g,b;     /* vertex color */
};

struct PlyFace
{
    unsigned char nverts;    /* number of vertex indices in list */
    int *verts;              /* vertex index list */
    unsigned char ntexcoord; /* number of tex coords */
    float *texcoord;         /* texture coordinates */
    double nx,ny,nz;         /* normal vector */
};

/* list of the kinds of elements in the user's object */
const char *elem_names[] =
{
    "vertex", "face"
};

ply::PlyProperty available_vertex_properties[] = {
    {"x", Float32, Float32, offsetof(PlyVertex,x), 0, 0, 0, 0},
    {"y", Float32, Float32, offsetof(PlyVertex,y), 0, 0, 0, 0},
    {"z", Float32, Float32, offsetof(PlyVertex,z), 0, 0, 0, 0},
    {"nx", Float32, Float32, offsetof(PlyVertex,nx), 0, 0, 0, 0},
    {"ny", Float32, Float32, offsetof(PlyVertex,ny), 0, 0, 0, 0},
    {"nz", Float32, Float32, offsetof(PlyVertex,nz), 0, 0, 0, 0},
    {"s", Float32, Float32, offsetof(PlyVertex,u), 0, 0, 0, 0},
    {"t", Float32, Float32, offsetof(PlyVertex,v), 0, 0, 0, 0},
    {"red", Uint8, Uint8, offsetof(PlyVertex,r), 0, 0, 0, 0},
    {"green", Uint8, Uint8, offsetof(PlyVertex,g), 0, 0, 0, 0},
    {"blue", Uint8, Uint8, offsetof(PlyVertex,b), 0, 0, 0, 0},
};

ply::PlyProperty available_face_properties[] = { /* list of property information for a face */
    {"vertex_indices", PLY_Int32, PLY_Int32, offsetof(PlyFace,verts), 1, Uint8, Uint8, offsetof(PlyFace,nverts)},
    {"texcoord", Float32, Float32, offsetof(PlyFace,texcoord), 1, Uint8, Uint8, offsetof(PlyFace,ntexcoord)},
};



}

namespace ntk
{

void Mesh::applyTransform(const Pose3D& pose)
{
    foreach_idx(i, vertices)
            vertices[i] = pose.cameraTransform(vertices[i]);
}

Point3f Mesh :: centerize()
{
    Point3f center(0,0,0);
    foreach_idx(i, vertices)
    {
        center += vertices[i];
    }
    center *= 1.0/vertices.size();

    foreach_idx(i, vertices)
    {
        vertices[i] -= center;
    }
    return center;
}

Point3f Mesh :: center() const
{
    Point3f center(0,0,0);
    foreach_idx(i, vertices)
    {
        center += vertices[i];
    }
    center *= 1.0/vertices.size();
    return center;
}

void Mesh::saveToPlyFile(const char* filename) const
{   
    if (texture.data)
    {
        std::string texture_filename = filename;
        if (texture_filename.size() > 3)
        {
            texture_filename.erase(texture_filename.size()-4, 4); // remove .ply
            texture_filename += ".png";
        }
        else
        {
            texture_filename += ".texture.png";
        }
        imwrite(texture_filename, texture);
    }

    std::ofstream ply_file (filename);
    ply_file << "ply\n";
    ply_file << "format ascii 1.0\n";
    ply_file << "element vertex " << vertices.size() << "\n";
    ply_file << "property float x\n";
    ply_file << "property float y\n";
    ply_file << "property float z\n";

    if (hasNormals())
    {
        ply_file << "property float nx\n";
        ply_file << "property float ny\n";
        ply_file << "property float nz\n";
    }

    if (hasTexcoords())
    {
        // put it twice, blender uses (s,t) and meshlab (u,v)
        ply_file << "property float s\n";
        ply_file << "property float t\n";
    }

    if (hasColors())
    {
        ply_file << "property uchar red\n";
        ply_file << "property uchar green\n";
        ply_file << "property uchar blue\n";
    }

    if (hasFaces())
    {
        ply_file << "element face " << faces.size() << "\n";
        ply_file << "property list uchar uint vertex_indices\n";
        // For meshlab wedges.
        if (hasTexcoords() || hasFaceTexcoords())
            ply_file << "property list uchar float texcoord\n";
    }


    ply_file << "end_header\n";

    foreach_idx(i, vertices)
    {
        ply_file << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z;

        if (hasNormals())
            ply_file << " " << (ntk::math::isnan(normals[i].x) ? 0 : normals[i].x)
                     << " " << (ntk::math::isnan(normals[i].y) ? 0 : normals[i].y)
                     << " " << (ntk::math::isnan(normals[i].z) ? 0 : normals[i].z);

        if (hasTexcoords())
        {
            ply_file << " " << texcoords[i].x << " " << texcoords[i].y;
        }

        if (hasColors())
            ply_file << " " << (int)colors[i][0] << " " << (int)colors[i][1] << " " << (int)colors[i][2];

        ply_file << "\n";
    }

    if (hasFaces())
    {
        foreach_idx(i, faces)
        {
            ply_file << faces[i].numVertices();
            for (unsigned j = 0; j < faces[i].numVertices(); ++j)
                ply_file << " " << faces[i].indices[j];

            if (hasTexcoords() && !hasFaceTexcoords())
            {
                ply_file << "\n6";
                for (unsigned j = 0; j < faces[i].numVertices(); ++j)
                {
                    ply_file << " " << texcoords[faces[i].indices[j]].x;
                    ply_file << " " << 1.0 - texcoords[faces[i].indices[j]].y;
                }
            }
            else if (hasFaceTexcoords())
            {
                ply_file << "\n6";
                for (unsigned j = 0; j < faces[i].numVertices(); ++j)
                {
                    ply_file << " " << face_texcoords[i].u[j];
                    ply_file << " " << 1.0 - face_texcoords[i].v[j];
                }
            }
            ply_file << "\n";
        }
    }

    ply_file.close();
}

void Mesh::loadFromPlyFile(const char* filename)
{
    vertices.clear();
    colors.clear();
    texcoords.clear();
    normals.clear();
    faces.clear();

    bool has_colors = false;
    bool has_normals = false;
    bool has_texcoords = false;
    bool has_faces = false;

    std::vector<ply::PlyProperty> vertex_properties;
    std::vector<ply::PlyProperty> face_properties;

    std::vector<PlyVertex> ply_vertices;
    std::vector<PlyFace> ply_faces;

    FILE* mesh_file = fopen(filename, "rb");
    int err = errno;
    if (err)
    {
        ntk_dbg(0) << "[ERROR] " << strerror(err);
    }
    ntk_ensure(mesh_file, "Could not open mesh file.");

    ply::PlyFile* ply_file = ply::read_ply(mesh_file);
    ntk_ensure(ply_file, "Could not parse mesh file.");

    for (int i = 0; i < ply_file->num_elem_types; i++)
    {
        /* prepare to read the i'th list of elements */
        int elem_count = 0;
        const char* elem_name = ply::setup_element_read_ply (ply_file, i, &elem_count);

        if (ply::equal_strings("vertex", elem_name))
        {
            /* create a vertex list to hold all the vertices */
            ply_vertices.resize(elem_count);

            /* set up for getting vertex elements */
            ply::setup_property_ply (ply_file, &available_vertex_properties[0]);
            ply::setup_property_ply (ply_file, &available_vertex_properties[1]);
            ply::setup_property_ply (ply_file, &available_vertex_properties[2]);

            if (has_property(ply_file, "vertex", "nx"))
            {
                has_normals = true;
                ply::setup_property_ply (ply_file, &available_vertex_properties[3]);
                ply::setup_property_ply (ply_file, &available_vertex_properties[4]);
                ply::setup_property_ply (ply_file, &available_vertex_properties[5]);
            }

            if (has_property(ply_file, "vertex", "s"))
            {
                has_texcoords = true;
                ply::setup_property_ply (ply_file, &available_vertex_properties[6]);
                ply::setup_property_ply (ply_file, &available_vertex_properties[7]);
            }

            if (has_property(ply_file, "vertex", "red"))
            {
                has_colors = true;
                ply::setup_property_ply (ply_file, &available_vertex_properties[8]);
                ply::setup_property_ply (ply_file, &available_vertex_properties[9]);
                ply::setup_property_ply (ply_file, &available_vertex_properties[10]);
            }

            /* grab all the vertex elements */
            for (int j = 0; j < elem_count; j++)
                ply::get_element_ply (ply_file, &ply_vertices[j]);
        }
        else if (ply::equal_strings("face", elem_name))
        {
            has_faces = true;

            /* create a list to hold all the face elements */
            ply_faces.resize(elem_count);

            /* set up for getting face elements */
            setup_property_ply (ply_file, &available_face_properties[0]);

            if (has_property(ply_file, "face", "texcoord"))
            {
                setup_property_ply (ply_file, &available_face_properties[1]);
            }

            // setup_property_ply (ply_file, &global::face_props[1]);

            /* grab all the face elements */
            for (int j = 0; j < elem_count; j++)
            {
                get_element_ply (ply_file, &ply_faces[j]);
            }
        }
    }
    fclose(mesh_file);

    vertices.resize(ply_vertices.size());
    if (has_colors) colors.resize(vertices.size());
    if (has_normals) normals.resize(vertices.size());

    foreach_idx(i, ply_vertices)
    {
        const PlyVertex& v = ply_vertices[i];
        vertices[i] = Point3f(v.x, v.y, v.z);
        if (has_colors)
            colors[i] = Vec3b(v.r, v.g, v.b);
        if (has_normals)
            normals[i] = Point3f(v.nx, v.ny, v.nz);
    }

    if (has_faces)
    {
        faces.resize(ply_faces.size());
        foreach_idx(i, ply_faces)
        {
            const PlyFace& f = ply_faces[i];
            ntk_ensure(f.nverts == 3, "Only triangles are supported.");
            for (int j = 0; j < f.nverts; ++j)
                faces[i].indices[j] = f.verts[j];
        }
    }

}

void Mesh:: clear()
{
    vertices.clear();
    colors.clear();
    normals.clear();
    texcoords.clear();
    faces.clear();
    texture = cv::Mat3b();
}

void Mesh :: addPointFromSurfel(const Surfel& surfel)
{
    vertices.push_back(surfel.location);
    colors.push_back(surfel.color);
    normals.push_back(surfel.normal);
}

void Mesh :: addSurfel(const Surfel& surfel)
{
    int idx = vertices.size();

    ntk_assert(cv::norm(surfel.normal)>0.9, "Normal must be normalized and valid!");

    Vec3f v1, v2;
    orthogonal_basis(v1, v2, surfel.normal);
    Point3f p0 = surfel.location + Point3f(v1 * surfel.radius);
    Point3f p1 = surfel.location + Point3f(v1 * (surfel.radius/2.0f) + v2 * surfel.radius);
    Point3f p2 = surfel.location + Point3f(v1 * (-surfel.radius/2.0f) + v2 * surfel.radius);
    Point3f p3 = surfel.location + Point3f(v1 * -surfel.radius);
    Point3f p4 = surfel.location + Point3f(v1 * (-surfel.radius/2.0f) + v2 * (-surfel.radius));
    Point3f p5 = surfel.location + Point3f(v1 * (surfel.radius/2.0f) + v2 * (-surfel.radius));
    vertices.push_back(p0);
    vertices.push_back(p1);
    vertices.push_back(p2);
    vertices.push_back(p3);
    vertices.push_back(p4);
    vertices.push_back(p5);

    for (int k = 0; k < 6; ++k)
        colors.push_back(surfel.color);

    for (int k = 0; k < 6; ++k)
        normals.push_back(surfel.normal);

    {
        Face f;
        f.indices[0] = idx+5;
        f.indices[1] = idx+0;
        f.indices[2] = idx+1;
        faces.push_back(f);
    }

    {
        Face f;
        f.indices[0] = idx+5;
        f.indices[1] = idx+1;
        f.indices[2] = idx+2;
        faces.push_back(f);
    }

    {
        Face f;
        f.indices[0] = idx+4;
        f.indices[1] = idx+5;
        f.indices[2] = idx+2;
        faces.push_back(f);
    }

    {
        Face f;
        f.indices[0] = idx+4;
        f.indices[1] = idx+2;
        f.indices[2] = idx+3;
        faces.push_back(f);
    }
}


  void generate_mesh_from_plane(Mesh& mesh, const ntk::Plane& plane, const cv::Point3f& center, float plane_size)
  {
    Point3f line1[2] = { Point3f(center.x-plane_size, center.y-plane_size, center.z-plane_size),
                         Point3f(center.x-plane_size, center.y+plane_size, center.z-plane_size) };
    Point3f line2[2] = { Point3f(center.x+plane_size, center.y-plane_size, center.z-plane_size),
                         Point3f(center.x+plane_size, center.y+plane_size, center.z-plane_size) };
    Point3f line3[2] = { Point3f(center.x-plane_size, center.y-plane_size, center.z+plane_size),
                         Point3f(center.x-plane_size, center.y+plane_size, center.z+plane_size) };
    Point3f line4[2] = { Point3f(center.x+plane_size, center.y-plane_size, center.z+plane_size),
                         Point3f(center.x+plane_size, center.y+plane_size, center.z+plane_size) };

    Point3f plane_p1 = plane.intersectionWithLine(line1[0], line1[1]);
    Point3f plane_p2 = plane.intersectionWithLine(line2[0], line2[1]);
    Point3f plane_p3 = plane.intersectionWithLine(line3[0], line3[1]);
    Point3f plane_p4 = plane.intersectionWithLine(line4[0], line4[1]);

    mesh.vertices.push_back(plane_p1);
    mesh.vertices.push_back(plane_p2);
    mesh.vertices.push_back(plane_p3);
    mesh.vertices.push_back(plane_p4);

    {
        Face f;
        f.indices[0] = 0;
        f.indices[1] = 1;
        f.indices[2] = 2;
        mesh.faces.push_back(f);
    }

    {
        Face f;
        f.indices[0] = 2;
        f.indices[1] = 1;
        f.indices[2] = 3;
        mesh.faces.push_back(f);
    }
}

  void generate_mesh_from_cube(Mesh& mesh, const ntk::Rect3f& cube)
  {

      const int links[12][3] = { {0, 1, 3},
                                 {0, 3, 2},

                                 {0, 5, 1},
                                 {0, 4, 5},

                                 {3, 1, 5},
                                 {3, 5, 7},

                                 {2, 3, 7},
                                 {2, 7, 6},

                                 {6, 5, 4},
                                 {6, 7, 5},

                                 {0, 2, 6},
                                 {0, 6, 4} };

      Point3f center(cube.x + cube.width/2.,cube.y + cube.height/2., cube.z + cube.depth/2.);

      Point3f cube_points[8];
      Point3f proj_cube_points[8];

      const double xvals [] = {center.x-(cube.width/2), center.x+(cube.width/2)};
      const double yvals [] = {center.y-(cube.height/2), center.y+(cube.height/2)};
      const double zvals [] = {center.z-(cube.depth/2), center.z+(cube.depth/2)};

      int first_vertex_index = mesh.vertices.size();
      for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
      {
        Point3f p(xvals[i], yvals[j], zvals[k]);
        mesh.vertices.push_back(p);
      }

      for (int f = 0; f < 12; ++f)
      {
        Face face;
        for (int i = 0; i < 3; ++i)
        {
          face.indices[i] = first_vertex_index + links[f][i];

        }
        mesh.faces.push_back(face);
      }
    }




void Mesh::addCube(const cv::Point3f& center, const cv::Point3f& sizes, const cv::Vec3b& color)
  {
    const int links[12][3] = { {0, 1, 3},
                               {0, 3, 2},

                               {0, 5, 1},
                               {0, 4, 5},

                               {3, 1, 5},
                               {3, 5, 7},

                               {2, 3, 7},
                               {2, 7, 6},

                               {6, 5, 4},
                               {6, 7, 5},

                               {0, 2, 6},
                               {0, 6, 4} };

    const bool has_colors = hasColors();

    Point3f cube_points[8];
    Point3f proj_cube_points[8];

    const double xvals [] = {center.x-(sizes.x/2), center.x+(sizes.x/2)};
    const double yvals [] = {center.y-(sizes.y/2), center.y+(sizes.y/2)};
    const double zvals [] = {center.z-(sizes.z/2), center.z+(sizes.z/2)};

    int first_vertex_index = vertices.size();
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            for (int k = 0; k < 2; ++k)
            {
                Point3f p(xvals[i], yvals[j], zvals[k]);
                vertices.push_back(p);
                if (has_colors)
                    colors.push_back(color);
            }

    for (int f = 0; f < 12; ++f)
    {
        Face face;
        for (int i = 0; i < 3; ++i)
        {
            face.indices[i] = first_vertex_index + links[f][i];
        }
        faces.push_back(face);
    }
}

void Mesh :: addMesh(const ntk::Mesh& rhs)
{
    if (vertices.size() < 1)
    {
        *this = rhs;
        return;
    }

    // must check it before modifying it.
    bool has_colors = hasColors();

    int offset = vertices.size();
    vertices.insert(vertices.end(), rhs.vertices.begin(), rhs.vertices.end());
    if (has_colors)
    {
        ntk_throw_exception_if(!rhs.hasColors(), "Cannot merge different kind of meshes.");
        colors.insert(colors.end(), rhs.colors.begin(), rhs.colors.end());
    }

    bool has_normals = hasNormals();
    if (has_normals)
    {
        ntk_throw_exception_if(!rhs.hasNormals(), "Cannot merge different kind of meshes.");
        normals.insert(normals.end(), rhs.normals.begin(), rhs.normals.end());
    }

    for (int i = 0; i < rhs.faces.size(); ++i)
    {
        Face face = rhs.faces[i];
        face.indices[0] += offset;
        face.indices[1] += offset;
        face.indices[2] += offset;
        faces.push_back(face);
    }
}

void Mesh::
applyScaleTransform(float x_scale, float y_scale, float z_scale)
{
    foreach_idx(i, vertices)
    {
        vertices[i].x *= x_scale;
        vertices[i].y *= y_scale;
        vertices[i].z *= z_scale;
    }
}

void Mesh::computeNormalsFromFaces()
{
    normals.clear();
    normals.resize(vertices.size(), Vec3f(0,0,0));
    foreach_idx(i, faces)
    {
        const Face& face = faces[i];
        Vec3f v01 = vertices[face.indices[1]] - vertices[face.indices[0]];
        Vec3f v02 = vertices[face.indices[2]] - vertices[face.indices[0]];
        Vec3f n = v01.cross(v02);
        for (int k = 0; k < face.numVertices(); ++k)
            normals[face.indices[k]] += Point3f(n);
    }

    foreach_idx(i, normals)
    {
        Vec3f v = normals[i];
        ntk::normalize(v);
        normals[i] = v;
    }
}

void Mesh::computeVertexFaceMap(std::vector< std::vector<int> >& faces_per_vertex)
{
    faces_per_vertex.resize(vertices.size());
    foreach_idx(face_i, faces)
    {
        for (int v_i = 0; v_i < 3; ++v_i)
        {
            faces_per_vertex[faces[face_i].indices[v_i]].push_back(face_i);
        }
    }
}

struct VertexComparator
{
    VertexComparator(const std::vector<cv::Point3f>& vertices) : vertices(vertices) {}

    bool operator()(int i1, int i2) const
    {
        return vertices[i1] < vertices[i2];
    }

    const std::vector<cv::Point3f>& vertices;
};

void Mesh::removeDuplicatedVertices()
{
    std::vector<int> ordered_indices (vertices.size());
    foreach_idx(i, ordered_indices) ordered_indices[i] = i;

    VertexComparator comparator (vertices);
    std::sort(stl_bounds(ordered_indices), comparator);

    std::map<int, int> vertex_alias;

    int i = 0;
    int j = i;
    for (; i < ordered_indices.size() - 1; )
    {
        j = i + 1;
        while (j < ordered_indices.size() && vertices[ordered_indices[i]] == vertices[ordered_indices[j]])
        {
            vertex_alias[ordered_indices[j]] = ordered_indices[i];
            vertices[ordered_indices[j]] = infinite_point();
            ++j;
        }
        i = j;
    }

    foreach_idx(face_i, faces)
    {
        foreach_idx(v_i, faces[face_i])
        {
            std::map<int, int>::const_iterator it = vertex_alias.find(faces[face_i].indices[v_i]);
            if (it != vertex_alias.end())
            {
                faces[face_i].indices[v_i] = it->second;
            }
        }
    }
}

void Mesh::removeIsolatedVertices()
{
    std::vector<int> new_indices (vertices.size());
    int cur_index = 0;
    foreach_idx(i, vertices)
    {
        if (isnan(vertices[i]))
        {
            new_indices[i] = -1;
        }
        else
        {
            new_indices[i] = cur_index;
            ++cur_index;
        }
    }

    ntk::Mesh new_mesh;
    new_mesh.vertices.resize(cur_index);

    if (hasColors())
        new_mesh.colors.resize(cur_index);
    if (hasNormals())
        new_mesh.normals.resize(cur_index);
    if (hasTexcoords())
        new_mesh.texcoords.resize(cur_index);

    foreach_idx(i, vertices)
    {
        if (new_indices[i] < 0) continue;
        new_mesh.vertices[new_indices[i]] = vertices[i];
        if (hasColors())
            new_mesh.colors[new_indices[i]] = colors[i];
        if (hasNormals())
            new_mesh.normals[new_indices[i]] = normals[i];
        if (hasTexcoords())
            new_mesh.texcoords[new_indices[i]] = texcoords[i];
    }

    foreach_idx(face_i, faces)
    {
        foreach_idx(v_i, faces[face_i])
        {
            int old_index = faces[face_i].indices[v_i];
            faces[face_i].indices[v_i] = new_indices[old_index];
        }
    }

    vertices = new_mesh.vertices;
    colors = new_mesh.colors;
    normals = new_mesh.normals;
    texcoords = new_mesh.texcoords;
}

} // end of ntk
