# 🏟️ Saclay Arena – Stadium & Fireworks

### OpenGL Computer Graphics Project

## 🎥 Demo

👉 **YouTube video:** [https://youtu.be/NKI_XAFT20A](https://youtu.be/NKI_XAFT20A)

## 📸 Screenshots

<img width="2564" height="1484" alt="Stadium view 1" src="https://github.com/user-attachments/assets/b21e8f4d-4f81-4667-8168-190356bb4616" />

<img width="2564" height="1484" alt="Stadium view 2" src="https://github.com/user-attachments/assets/25f2d3b5-523c-433a-910c-441341b72cf3" />

<img width="2564" height="1484" alt="Stadium view 3" src="https://github.com/user-attachments/assets/e8d09994-387e-459b-b84e-9216886cbac4" />

<img width="2564" height="1484" alt="Fireworks and atmosphere" src="https://github.com/user-attachments/assets/75951017-ffba-466c-8e1d-98532f40df3b" />

---

## 📌 Project Overview

As part of the **Introduction to Computer Graphics** course, we developed a complete **3D football stadium scene** using **Blender** for modeling and **OpenGL** for rendering.

This project is strongly inspired by a real-world context: **our country hosting a major sporting event, the Africa Cup of Nations**. It represents a fusion between our **passion for sport** and our **academic identity**, transforming a classic football stadium into a venue proudly featuring the colors of **Université Paris-Saclay** and **Polytech**.

---

## 🧱 3D Modeling with Blender

Most of the visual elements in the scene were **entirely modeled in Blender**. Our work focused on creating original assets while maintaining a consistent visual style.

### 🐂 The PPS Mascot

The **central element** of the project is the **Polytech Paris-Saclay mascot (the bull)**.

* Modeled from scratch in Blender
* Used as a base model for **players and the referee**
* Required careful handling of **organic shapes** to preserve its **cartoon-like appearance**

This choice allowed us to unify the visual identity of the scene while strongly embedding the school’s branding into the project.

---

## 🎨 Texturing & Materials

Texturing played a key role in personalizing and enhancing realism.

### Texture Mapping

* The **school logo** was mapped directly onto the **center of the pitch**
* **Université Paris-Saclay flags** were placed on advertising panels around the field

### Materials & Colors

* Stadium stands colored **blue**, reflecting the university’s identity
* Football pitch uses a **green checkerboard shader** to simulate a realistic grass pattern

### Flags

* Custom textures displaying **“Université Paris-Saclay”** were applied to all flagpoles surrounding the stadium

---

## 📘 Learning Blender

This project marked our **first experience with Blender**.

At first, the software felt complex due to its rich feature set. However, as the project progressed, we:

* Gained confidence with the Blender workflow
* Learned efficient modeling and texturing techniques
* Discovered the satisfaction of bringing a 3D scene to life visually

Overall, Blender proved to be a powerful and enjoyable tool for computer graphics creation.

---

## ⚙️ Importing Models into OpenGL

After modeling, we moved on to rendering the scene in **OpenGL**.

To import complex 3D models efficiently, we used **Assimp (Open Asset Import Library)**.

### Import Pipeline Progression

1. We have started with a imple model: 🍋 *a lemon* (so we can learn)
2. Textured football
3. Polytech mascot
4. Full stadium with all components

The final stadium model contains **over 100 meshes**, making it an excellent stress test for our import pipeline and rendering system.

---

## 🧠 Technologies Used

* **Blender** – 3D modeling & texturing
* **OpenGL** – Real-time rendering
* **Assimp** – Model import library
* **C++** – Core application logic

---

## ✨ Conclusion

This project allowed us to combine **creativity, technical skills, and real-time graphics** into a single cohesive experience. It strengthened our understanding of the complete 3D graphics pipeline, from modeling to real-time rendering, while celebrating both **sport and university identity**.

---

📌 *If you like the project, feel free to leave a ⭐ on the repository!*
