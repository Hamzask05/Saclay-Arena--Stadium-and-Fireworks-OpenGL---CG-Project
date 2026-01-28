#include "textured_sphere.h"

TexturedSphere::TexturedSphere(Shader *shader_program, Texture *texture)
    : Sphere(shader_program), texture(texture)
{
    loc_diffuse_map = glGetUniformLocation(this->shader_program_, "diffuse_map");
}

void TexturedSphere::draw(glm::mat4& model, glm::mat4& view, glm::mat4& projection) {

    glUseProgram(this->shader_program_);

    // TODO activate, bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->getGLid());

    glUniform1i(loc_diffuse_map, 0); // TODO send the correct texture to the shader

    Sphere::draw(model, view, projection);

    // TODO Unbind the texture
     glBindTexture(GL_TEXTURE_2D, 0);



    glUseProgram(0);
}
