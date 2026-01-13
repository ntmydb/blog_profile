import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext
import subprocess
import threading
import os
import sys
import platform

class Video2GifApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Video2Gif GUI")
        self.root.geometry("700x800")
        
        self.input_file = tk.StringVar()
        self.output_file = tk.StringVar()
        
        self.create_widgets()
        self.find_executable()

    def find_executable(self):
        # Try to find the executable in common build locations
        possible_paths = [
            os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "bin", "video2gif"),
            os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "video2gif"),
            os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "bin", "video2gif"),
            "video2gif" # If in PATH
        ]
        
        if platform.system() == "Windows":
            possible_paths = [p + ".exe" for p in possible_paths]

        self.executable_path = None
        for path in possible_paths:
            if os.path.exists(path) and os.path.isfile(path):
                self.executable_path = path
                break
        
        if self.executable_path:
            self.log(f"Found executable at: {self.executable_path}")
        else:
            self.log("Warning: video2gif executable not found. Please build the project first.")

    def create_widgets(self):
        # Main container
        main_frame = tk.Frame(self.root, padx=20, pady=20)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # Title
        tk.Label(main_frame, text="Video转GIF转换器", font=("Helvetica", 16, "bold")).pack(pady=(0, 20))

        # File Selection
        file_frame = tk.LabelFrame(main_frame, text="文件选择", padx=10, pady=10)
        file_frame.pack(fill=tk.X, pady=(0, 10))

        # Input File
        tk.Label(file_frame, text="输入视频文件路径:").grid(row=0, column=0, sticky="w")
        tk.Entry(file_frame, textvariable=self.input_file, width=40).grid(row=0, column=1, padx=5)
        tk.Button(file_frame, text="浏览文件...", command=self.browse_input).grid(row=0, column=2)

        # Output File
        tk.Label(file_frame, text="输出GIF文件路径:").grid(row=1, column=0, sticky="w", pady=(10, 0))
        tk.Entry(file_frame, textvariable=self.output_file, width=40).grid(row=1, column=1, padx=5, pady=(10, 0))
        tk.Button(file_frame, text="浏览文件...", command=self.browse_output).grid(row=1, column=2, pady=(10, 0))

        # Configuration
        config_frame = tk.LabelFrame(main_frame, text="配置", padx=10, pady=10)
        config_frame.pack(fill=tk.X, pady=(0, 10))

        # Grid layout for config
        # Width
        tk.Label(config_frame, text="宽:").grid(row=0, column=0, sticky="w")
        self.width_var = tk.IntVar(value=0)
        tk.Entry(config_frame, textvariable=self.width_var, width=10).grid(row=0, column=1, sticky="w", padx=5)
        tk.Label(config_frame, text="(输入0表示保持原始)").grid(row=0, column=2, sticky="w")

        # Height
        tk.Label(config_frame, text="高:").grid(row=0, column=3, sticky="w", padx=(20, 0))
        self.height_var = tk.IntVar(value=0)
        tk.Entry(config_frame, textvariable=self.height_var, width=10).grid(row=0, column=4, sticky="w", padx=5)
        tk.Label(config_frame, text="(输入0表示保持原始)").grid(row=0, column=5, sticky="w")

        # FPS
        tk.Label(config_frame, text="FPS(帧率):").grid(row=1, column=0, sticky="w", pady=10)
        self.fps_var = tk.IntVar(value=10)
        tk.Entry(config_frame, textvariable=self.fps_var, width=10).grid(row=1, column=1, sticky="w", padx=5, pady=10)

        # Quality
        tk.Label(config_frame, text="动图质量 (1-100):").grid(row=1, column=3, sticky="w", padx=(20, 0), pady=10)
        self.quality_var = tk.IntVar(value=100)
        tk.Scale(config_frame, from_=1, to=100, orient=tk.HORIZONTAL, variable=self.quality_var, length=150).grid(row=1, column=4, columnspan=2, sticky="w", pady=10)

        # Start Time
        tk.Label(config_frame, text="起始时间 (s):").grid(row=2, column=0, sticky="w")
        self.start_var = tk.DoubleVar(value=0.0)
        tk.Entry(config_frame, textvariable=self.start_var, width=10).grid(row=2, column=1, sticky="w", padx=5)

        # Duration
        tk.Label(config_frame, text="截取总时长(s):").grid(row=2, column=3, sticky="w", padx=(20, 0))
        self.duration_var = tk.DoubleVar(value=0.0)
        tk.Entry(config_frame, textvariable=self.duration_var, width=10).grid(row=2, column=4, sticky="w", padx=5)
        tk.Label(config_frame, text="(0表示整个视频文件全部转换)").grid(row=2, column=5, sticky="w")

        # Checkboxes
        self.no_aspect_var = tk.BooleanVar(value=False)
        tk.Checkbutton(config_frame, text="忽略长宽比", variable=self.no_aspect_var).grid(row=3, column=0, columnspan=3, sticky="w", pady=(10, 0))

        # Actions
        action_frame = tk.Frame(main_frame)
        action_frame.pack(fill=tk.X, pady=(0, 10))
        
        self.convert_btn = tk.Button(action_frame, text="转换", command=self.start_conversion, bg="#4CAF50", fg="white", font=("Helvetica", 12, "bold"), height=2)
        self.convert_btn.pack(fill=tk.X)

        # Log Area
        log_frame = tk.LabelFrame(main_frame, text="日志", padx=10, pady=10)
        log_frame.pack(fill=tk.BOTH, expand=True)
        
        self.log_area = scrolledtext.ScrolledText(log_frame, height=15)
        self.log_area.pack(fill=tk.BOTH, expand=True)

    def browse_input(self):
        filename = filedialog.askopenfilename(filetypes=[("Video files", "*.mp4 *.avi *.mov *.mkv *.wmv *.flv *.webm"), ("All files", "*.*")])
        if filename:
            self.input_file.set(filename)
            # Auto-set output file
            base, _ = os.path.splitext(filename)
            self.output_file.set(base + ".gif")

    def browse_output(self):
        filename = filedialog.asksaveasfilename(defaultextension=".gif", filetypes=[("GIF files", "*.gif"), ("All files", "*.*")])
        if filename:
            self.output_file.set(filename)

    def log(self, message):
        self.log_area.insert(tk.END, message + "\n")
        self.log_area.see(tk.END)

    def start_conversion(self):
        if not self.executable_path:
            messagebox.showerror("Error", "未找到可执行文件，请先编译项目")
            return

        input_path = self.input_file.get()
        output_path = self.output_file.get()

        if not input_path or not os.path.exists(input_path):
            messagebox.showerror("Error", "请选择一个视频文件")
            return
        
        if not output_path:
            messagebox.showerror("Error", "请选择一个输出文件路径")
            return

        # Build command
        cmd = [
            self.executable_path,
            "-w", str(self.width_var.get()),
            "-h", str(self.height_var.get()),
            "-f", str(self.fps_var.get()),
            "-s", str(self.start_var.get()),
            "-d", str(self.duration_var.get()),
            "-q", str(self.quality_var.get()),
            input_path,
            output_path
        ]

        if self.no_aspect_var.get():
            cmd.insert(1, "--no-aspect")

        self.convert_btn.config(state=tk.DISABLED)
        self.log("-" * 50)
        self.log(f"Starting conversion: {' '.join(cmd)}")
        
        # Run in separate thread
        threading.Thread(target=self.run_process, args=(cmd,), daemon=True).start()

    def run_process(self, cmd):
        try:
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1
            )

            for line in process.stdout:
                self.root.after(0, self.log, line.strip())

            process.wait()
            
            if process.returncode == 0:
                self.root.after(0, lambda: messagebox.showinfo("成功", "转换成功!"))
                self.root.after(0, lambda: self.log("转换成功!"))
            else:
                self.root.after(0, lambda: messagebox.showerror("Error", f"转换失败，错误码： {process.returncode}"))
                self.root.after(0, lambda: self.log(f"转换失败，错误码： {process.returncode}"))

        except Exception as e:
            self.root.after(0, lambda: messagebox.showerror("Error", str(e)))
            self.root.after(0, lambda: self.log(f"Exception: {str(e)}"))
        
        finally:
            self.root.after(0, lambda: self.convert_btn.config(state=tk.NORMAL))

if __name__ == "__main__":
    root = tk.Tk()
    app = Video2GifApp(root)
    root.mainloop()