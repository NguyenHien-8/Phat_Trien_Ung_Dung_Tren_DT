# Supabase Database

```sql
-- ==========================================
-- BƯỚC 1: TẠO DỮ LIỆU LỚP HỌC
-- Phải có lớp học trong bảng public.classes thì mới có class_id để phân công
-- ==========================================
INSERT INTO public.classes (class_name, academic_year) 
VALUES 
    ('10A1', '2024-2025'),
    ('10A2', '2024-2025'),
    ('10A3', '2024-2025')
ON CONFLICT (class_name, academic_year) DO NOTHING;

-- ==========================================
-- BƯỚC 2: THÊM DỮ LIỆU VÀO BẢNG TEACHER_CLASS (Phân công lớp)
-- LƯU Ý QUAN TRỌNG: Các mã 'TNHGV01', 'TNHGV02' phải là các giáo viên đã ĐĂNG KÝ TÀI KHOẢN thành công và có mặt trong bảng public.teachers.
-- ==========================================

-- 1. Phân công giáo viên mã 'TNHGV01' dạy lớp '10A1' năm '2024-2025'
INSERT INTO public.teacher_class (teacher_id, class_id)
SELECT 'TNHGV01', id 
FROM public.classes 
WHERE class_name = '10A1' AND academic_year = '2024-2025'
ON CONFLICT (teacher_id, class_id) DO NOTHING;

-- 2. Phân công giáo viên mã 'TNHGV02' dạy thêm lớp '10A2' năm '2024-2025'
INSERT INTO public.teacher_class (teacher_id, class_id)
SELECT 'TNHGV02', id 
FROM public.classes 
WHERE class_name = '10A2' AND academic_year = '2024-2025'
ON CONFLICT (teacher_id, class_id) DO NOTHING;

-- 3. Phân công giáo viên mã 'TNHGV03' dạy lớp '10A3' năm '2024-2025'
INSERT INTO public.teacher_class (teacher_id, class_id)
SELECT 'TNHGV03', id 
FROM public.classes 
WHERE class_name = '10A3' AND academic_year = '2024-2025'
ON CONFLICT (teacher_id, class_id) DO NOTHING;

-- ==========================================
--  BƯỚC 3: KIỂM TRA LẠI KẾT QUẢ
-- ==========================================
SELECT 
    tc.id AS assignment_id,
    t.name AS teacher_name,
    tc.teacher_id,
    c.class_name,
    c.academic_year
FROM public.teacher_class tc
JOIN public.teachers t ON tc.teacher_id = t.teacher_id
JOIN public.classes c ON tc.class_id = c.id;

-- ==========================================
-- BƯỚC 4: THÊM DỮ LIỆU HỌC SINH VÀO BẢNG STUDENTS
-- ==========================================

-- 1. Thêm học sinh vào lớp 10A1
INSERT INTO public.students (student_id, name, class_id, fingerprint_id)
SELECT 'HS10A1_001', 'Nguyễn Văn An', id, '1'
FROM public.classes 
WHERE class_name = '10A1' AND academic_year = '2024-2025'
ON CONFLICT (student_id) DO NOTHING;

INSERT INTO public.students (student_id, name, class_id, fingerprint_id)
SELECT 'HS10A1_002', 'Trần Thị Bình', id, '2'
FROM public.classes 
WHERE class_name = '10A1' AND academic_year = '2024-2025'
ON CONFLICT (student_id) DO NOTHING;

-- 2. Thêm học sinh vào lớp 10A2
INSERT INTO public.students (student_id, name, class_id, fingerprint_id)
SELECT 'HS10A2_001', 'Lê Hoàng Cường', id, '3'
FROM public.classes 
WHERE class_name = '10A2' AND academic_year = '2024-2025'
ON CONFLICT (student_id) DO NOTHING;

INSERT INTO public.students (student_id, name, class_id, fingerprint_id)
SELECT 'HS10A2_002', 'Phạm Mỹ Dung', id, '4'
FROM public.classes 
WHERE class_name = '10A2' AND academic_year = '2024-2025'
ON CONFLICT (student_id) DO NOTHING;

-- 3. Thêm học sinh vào lớp 10A3
INSERT INTO public.students (student_id, name, class_id, fingerprint_id)
SELECT 'HS10A3_001', 'Đỗ Khắc Duy', id, '5'
FROM public.classes 
WHERE class_name = '10A3' AND academic_year = '2024-2025'
ON CONFLICT (student_id) DO NOTHING;


-- ==========================================
-- BƯỚC 5: KIỂM TRA LẠI DANH SÁCH HỌC SINH VÀ LỚP HỌC
-- ==========================================
SELECT 
    s.student_id,
    s.name AS student_name,
    c.class_name,
    c.academic_year,
    s.fingerprint_id
FROM public.students s
JOIN public.classes c ON s.class_id = c.id
ORDER BY c.class_name, s.student_id;
```