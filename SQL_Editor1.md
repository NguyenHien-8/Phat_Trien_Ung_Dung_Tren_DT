# Supabase Database

```sql
-- Kích hoạt extension pgcrypto (nếu chưa có) để mã hóa password
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- Khởi tạo các giá trị token mặc định cho user hiện tại (tránh lỗi null)
UPDATE auth.users 
SET 
    confirmation_token = COALESCE(confirmation_token, ''), 
    recovery_token = COALESCE(recovery_token, ''), 
    email_change_token_new = COALESCE(email_change_token_new, ''), 
    email_change = COALESCE(email_change, '');

-- ==========================================
-- PHẦN 1: CẤU TRÚC BẢNG VÀ USER AUTH
-- ==========================================

CREATE TABLE IF NOT EXISTS public.teachers (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    teacher_id TEXT UNIQUE NOT NULL,
    email TEXT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    user_id UUID UNIQUE REFERENCES auth.users(id) ON DELETE CASCADE
);

DROP TABLE IF EXISTS public.teacher_ids CASCADE;

CREATE TABLE IF NOT EXISTS public.teacher_ids (
    id SERIAL PRIMARY KEY,
    teacher_id TEXT UNIQUE NOT NULL,
    email TEXT UNIQUE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Cập nhật data mẫu cho giáo viên
INSERT INTO public.teacher_ids (teacher_id, email) VALUES
('TNHGV01', 'trannguyenhien29085@gmail.com'), 
('TNHGV02', 'nhanb2305247@student.ctu.edu.vn'), 
('TNHGV03', 'toanb2305266@student.ctu.edu.vn'), 
('TNHGV04', 'tilb2305264@student.ctu.edu.vn'), 
('TNHGV05', 'longb2305238@student.ctu.edu.vn')
ON CONFLICT (teacher_id) DO NOTHING;

DROP FUNCTION IF EXISTS public.signup_teacher(TEXT, TEXT, TEXT, TEXT);

-- ==========================================
-- HÀM SIGN UP (XỬ LÝ ĐĂNG KÝ GIÁO VIÊN)
-- ==========================================
CREATE OR REPLACE FUNCTION public.signup_teacher(
    p_teacher_id TEXT, 
    p_email TEXT, 
    p_password TEXT, 
    p_name TEXT
) RETURNS JSONB LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_db_email TEXT; v_db_teacher_id TEXT; v_new_user_id UUID;
BEGIN
    IF EXISTS (SELECT 1 FROM auth.users WHERE email = p_email) OR 
       EXISTS (SELECT 1 FROM public.teachers WHERE teacher_id = p_teacher_id) THEN
        RETURN jsonb_build_object('success', false, 'message', 'The teacher has already been registered.');
    END IF;

    SELECT email INTO v_db_email FROM public.teacher_ids WHERE teacher_id = p_teacher_id;
    SELECT teacher_id INTO v_db_teacher_id FROM public.teacher_ids WHERE email = p_email;

    IF v_db_email IS NULL AND v_db_teacher_id IS NULL THEN
        RETURN jsonb_build_object('success', false, 'message', 'Both The Email And Teacher Id Are Incorrect');
    ELSIF v_db_email IS NOT NULL AND v_db_email != p_email THEN
        RETURN jsonb_build_object('success', false, 'message', 'Incorrect Email Sign Up');
    ELSIF v_db_teacher_id IS NOT NULL AND v_db_teacher_id != p_teacher_id THEN
        RETURN jsonb_build_object('success', false, 'message', 'Incorrect Teacher Id Sign Up');
    ELSIF v_db_email = p_email AND v_db_teacher_id = p_teacher_id THEN
        v_new_user_id := gen_random_uuid();
        
        -- Insert vào auth.users 
        INSERT INTO auth.users (
            instance_id, id, aud, role, email, encrypted_password, email_confirmed_at, 
            raw_app_meta_data, raw_user_meta_data, created_at, updated_at,
            confirmation_token, recovery_token, email_change_token_new, email_change
        ) VALUES (
            '00000000-0000-0000-0000-000000000000', v_new_user_id, 'authenticated', 'authenticated', 
            p_email, crypt(p_password, gen_salt('bf')), NOW(), 
            '{"provider":"email","providers":["email"]}'::jsonb, 
            jsonb_build_object('teacher_id', p_teacher_id, 'name', p_name), NOW(), NOW(),
            '', '', '', ''
        );

        -- Thêm vào auth.identities
        INSERT INTO auth.identities (
            id, user_id, provider_id, identity_data, provider, last_sign_in_at, created_at, updated_at
        ) VALUES (
            gen_random_uuid(), v_new_user_id, v_new_user_id::text, 
            jsonb_build_object('sub', v_new_user_id, 'email', p_email, 'email_verified', true, 'phone_verified', false),
            'email', NOW(), NOW(), NOW()
        );

        RETURN jsonb_build_object('success', true, 'message', 'Sign Up Success');
    ELSE
        RETURN jsonb_build_object('success', false, 'message', 'Invalid credentials');
    END IF;
EXCEPTION WHEN OTHERS THEN
    RETURN jsonb_build_object('success', false, 'message', 'Database Error: ' || SQLERRM);
END;
$$;

DROP FUNCTION IF EXISTS public.signin_with_email(TEXT, TEXT);

-- ==========================================
-- HÀM SIGN IN (XỬ LÝ ĐĂNG NHẬP)
-- ==========================================
CREATE OR REPLACE FUNCTION public.signin_with_email(
    p_email TEXT, p_password TEXT
) RETURNS JSONB LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_user_id UUID; v_encrypted_pw TEXT; v_teacher_id TEXT;
BEGIN
    SELECT u.id, u.encrypted_password, t.teacher_id 
    INTO v_user_id, v_encrypted_pw, v_teacher_id
    FROM auth.users u
    LEFT JOIN public.teachers t ON u.id = t.user_id
    WHERE u.email = p_email;

    IF v_user_id IS NULL THEN 
        RETURN jsonb_build_object('success', false, 'message', 'Incorrect Email Sign In'); 
    END IF;

    IF v_encrypted_pw = crypt(p_password, v_encrypted_pw) THEN
        RETURN jsonb_build_object('success', true, 'message', 'Sign In Success', 'email', p_email, 'teacher_id', v_teacher_id);
    ELSE
        RETURN jsonb_build_object('success', false, 'message', 'Incorrect Password Sign In');
    END IF;
END;
$$;

DROP TRIGGER IF EXISTS on_auth_user_created ON auth.users;

-- Trigger tự động thêm giáo viên vào bảng public.teachers
CREATE OR REPLACE FUNCTION public.handle_new_user() RETURNS TRIGGER LANGUAGE plpgsql SECURITY DEFINER AS $$
DECLARE
    v_teacher_id TEXT; v_name TEXT;
BEGIN
    v_teacher_id := NEW.raw_user_meta_data->>'teacher_id';
    v_name := NEW.raw_user_meta_data->>'name';
    INSERT INTO public.teachers (user_id, teacher_id, email, name)
    VALUES (NEW.id, v_teacher_id, NEW.email, v_name);
    RETURN NEW;
END;
$$;

CREATE TRIGGER on_auth_user_created
    AFTER INSERT ON auth.users
    FOR EACH ROW EXECUTE FUNCTION public.handle_new_user();

-- ==========================================
-- PHẦN 2: BẢNG LỚP HỌC VÀ HỌC SINH
-- ==========================================

CREATE TABLE IF NOT EXISTS public.classes (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    class_name TEXT NOT NULL,
    academic_year TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(class_name, academic_year)
);

CREATE TABLE IF NOT EXISTS public.teacher_class (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    teacher_id TEXT NOT NULL REFERENCES public.teachers(teacher_id) ON DELETE CASCADE,
    class_id UUID NOT NULL REFERENCES public.classes(id) ON DELETE CASCADE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    UNIQUE(teacher_id, class_id)
);

CREATE TABLE IF NOT EXISTS public.students (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    student_id TEXT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    class_id UUID NOT NULL REFERENCES public.classes(id) ON DELETE CASCADE,
    fingerprint_id TEXT UNIQUE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- ==========================================
-- PHẦN 3: THIẾT LẬP PHÂN QUYỀN RLS CƠ BẢN
-- ==========================================
ALTER TABLE public.classes ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.teacher_class ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.students ENABLE ROW LEVEL SECURITY;

DROP POLICY IF EXISTS "Teacher views assigned classes" ON public.classes;
DROP POLICY IF EXISTS "Teacher views own assignments" ON public.teacher_class;
DROP POLICY IF EXISTS "Teacher views students in assigned classes" ON public.students;
DROP POLICY IF EXISTS "Teacher can insert students to assigned classes" ON public.students;
DROP POLICY IF EXISTS "Teacher can update students in assigned classes" ON public.students;
DROP POLICY IF EXISTS "Teacher can delete students in assigned classes" ON public.students;

CREATE POLICY "Teacher views assigned classes" ON public.classes FOR SELECT TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = classes.id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher views own assignments" ON public.teacher_class FOR SELECT TO authenticated USING (EXISTS (SELECT 1 FROM public.teachers t WHERE t.teacher_id = teacher_class.teacher_id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher views students in assigned classes" ON public.students FOR SELECT TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));

CREATE POLICY "Teacher can insert students to assigned classes" ON public.students FOR INSERT TO authenticated WITH CHECK (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher can update students in assigned classes" ON public.students FOR UPDATE TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));
CREATE POLICY "Teacher can delete students in assigned classes" ON public.students FOR DELETE TO authenticated USING (EXISTS (SELECT 1 FROM public.teacher_class tc JOIN public.teachers t ON tc.teacher_id = t.teacher_id WHERE tc.class_id = students.class_id AND t.user_id = auth.uid()));

-- ==========================================
-- PHẦN 4: HÀM ĐỒNG BỘ HỌC SINH (SYNC STUDENTS)
-- ==========================================
CREATE OR REPLACE FUNCTION public.sync_students(payload JSONB)
RETURNS JSONB
LANGUAGE plpgsql
SECURITY INVOKER
AS $$
-- (Giữ nguyên logic của bạn)
DECLARE
    item JSONB; v_action TEXT; v_student_id TEXT; v_name TEXT; v_class_id UUID; v_fingerprint_id TEXT;
    v_existing_id UUID; v_existing_name TEXT; v_existing_fingerprint TEXT; v_existing_class_id UUID;
    results JSONB := '[]'::JSONB;
BEGIN
    IF auth.uid() IS NULL THEN RETURN jsonb_build_object('success', false, 'message', 'Unauthorized user.'); END IF;

    FOR item IN SELECT * FROM jsonb_array_elements(payload)
    LOOP
        v_action := item->>'action'; v_student_id := item->>'student_id'; v_name := item->>'name';
        v_class_id := (item->>'class_id')::UUID; v_fingerprint_id := item->>'fingerprint_id';

        IF v_action = 'delete' THEN
            DELETE FROM public.students WHERE student_id = v_student_id;
            results := results || jsonb_build_object('student_id', v_student_id, 'status', 'deleted', 'message', 'The student has been removed.');
        ELSIF v_action = 'save' THEN
            SELECT id, name, fingerprint_id, class_id INTO v_existing_id, v_existing_name, v_existing_fingerprint, v_existing_class_id
            FROM public.students WHERE student_id = v_student_id;

            IF NOT FOUND THEN
                BEGIN
                    INSERT INTO public.students (student_id, name, class_id, fingerprint_id) VALUES (v_student_id, v_name, v_class_id, v_fingerprint_id);
                    results := results || jsonb_build_object('student_id', v_student_id, 'status', 'inserted', 'message', 'Student information has been added.');
                EXCEPTION WHEN unique_violation THEN
                    IF SQLERRM LIKE '%student_id%' THEN results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The student code has already been duplicated');
                    ELSE results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The fingerprint code has already been duplicated.'); END IF;
                END;
            ELSE
                IF v_existing_name = v_name AND v_existing_fingerprint = v_fingerprint_id AND v_existing_class_id = v_class_id THEN
                    results := results || jsonb_build_object('student_id', v_student_id, 'status', 'unchanged', 'message', 'The student is information is already in the system.');
                ELSE
                    BEGIN
                        UPDATE public.students SET name = v_name, class_id = v_class_id, fingerprint_id = v_fingerprint_id WHERE student_id = v_student_id;
                        results := results || jsonb_build_object('student_id', v_student_id, 'status', 'updated', 'message', 'Student information has been updated.');
                    EXCEPTION WHEN unique_violation THEN
                        IF SQLERRM LIKE '%student_id%' THEN results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The student code has already been duplicated');
                        ELSE results := results || jsonb_build_object('student_id', v_student_id, 'status', 'error', 'message', 'The fingerprint code has already been duplicated.'); END IF;
                    END;
                END IF;
            END IF;
        END IF;
    END LOOP;
    RETURN jsonb_build_object('success', true, 'data', results);
END;
$$;

-- ==========================================
-- BỔ SUNG 1: BẢNG QUERY_FINGERPRINT & CẬP NHẬT RLS MỚI
-- ==========================================
CREATE TABLE IF NOT EXISTS public.query_fingerprint (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    student_id TEXT UNIQUE NOT NULL REFERENCES public.students(student_id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    fingerprint_id TEXT UNIQUE NOT NULL,
    fingerprint_data TEXT DEFAULT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- BẬT RLS VÀ PHÂN QUYỀN LẠI CHO TRUY VẤN TỪ APP CỦA TEACHER
ALTER TABLE public.query_fingerprint ENABLE ROW LEVEL SECURITY;
DROP POLICY IF EXISTS "Cho phép đọc dữ liệu query_fingerprint" ON public.query_fingerprint;
DROP POLICY IF EXISTS "Teacher views query_fingerprint of assigned classes" ON public.query_fingerprint;

-- Policy cốt lõi: Giáo viên (chứa auth.uid() trong access_token) chỉ được thấy học sinh thuộc lớp mình
CREATE POLICY "Teacher views query_fingerprint of assigned classes" ON public.query_fingerprint 
FOR SELECT TO authenticated 
USING (
    EXISTS (
        SELECT 1 
        FROM public.students s
        JOIN public.teacher_class tc ON s.class_id = tc.class_id
        JOIN public.teachers t ON tc.teacher_id = t.teacher_id
        WHERE s.student_id = query_fingerprint.student_id 
          AND t.user_id = auth.uid()
    )
);

-- HÀM ĐỂ ESP32 GỌI LÊN (SECURITY DEFINER giúp ESP32 bỏ qua RLS khi Update dữ liệu)
CREATE OR REPLACE FUNCTION public.sync_sensor_fingerprints(sensor_ids TEXT[])
RETURNS JSONB 
LANGUAGE plpgsql 
SECURITY DEFINER 
AS $$
BEGIN
    INSERT INTO public.query_fingerprint (student_id, name, fingerprint_id)
    SELECT student_id, name, fingerprint_id FROM public.students
    ON CONFLICT (student_id) DO UPDATE
    SET name = EXCLUDED.name, fingerprint_id = EXCLUDED.fingerprint_id;

    UPDATE public.query_fingerprint
    SET fingerprint_data = 'Registered'
    WHERE fingerprint_id = ANY(sensor_ids);

    UPDATE public.query_fingerprint
    SET fingerprint_data = 'Not Registered'
    WHERE NOT (fingerprint_id = ANY(sensor_ids)) OR fingerprint_data IS NULL;

    RETURN jsonb_build_object('success', true, 'message', 'Đã đồng bộ trạng thái vân tay thành công.');
END;
$$;

-- ==========================================
-- BỔ SUNG 2: BẢNG LƯU LỆNH ĐIỀU KHIỂN & BẬT REALTIME (WEBSOCKETS)
-- ==========================================
CREATE TABLE IF NOT EXISTS public.device_commands (
    id UUID DEFAULT gen_random_uuid() PRIMARY KEY,
    command TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_publication WHERE pubname = 'supabase_realtime') THEN
        CREATE PUBLICATION supabase_realtime;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_publication_tables WHERE pubname = 'supabase_realtime' AND tablename = 'device_commands') THEN
        ALTER PUBLICATION supabase_realtime ADD TABLE public.device_commands;
    END IF;
END
$$;

CREATE OR REPLACE FUNCTION public.request_sync()
RETURNS JSONB
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
    INSERT INTO public.device_commands (command) VALUES ('SYNC_FINGERPRINTS');
    RETURN jsonb_build_object('success', true, 'message', 'Đã gửi yêu cầu đồng bộ vân tay bằng WebSockets đến ESP32 thành công.');
END;
$$;
```