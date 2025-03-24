import tkinter as tk
from tkinter import filedialog, scrolledtext, ttk, messagebox
import subprocess
import os
import sys
import re
import time

class RISCVSimulatorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("RISC-V Simulator")
        self.root.geometry("1000x700")
        self.root.configure(bg="#f0f0f0")
        
        # Set paths
        self.setup_paths()
        
        # Current file path
        self.current_file = None
        self.simulation_running = False
        
        # Create main frames
        self.create_menu()
        self.create_toolbar()
        self.create_main_area()
        self.create_status_bar()
    
    def setup_paths(self):
        # Path to the C++ executable - adjust this based on your build setup
        if getattr(sys, 'frozen', False):
            # If running as compiled executable
            self.base_path = os.path.dirname(sys.executable)
        else:
            # If running as script
            self.base_path = os.path.dirname(os.path.abspath(__file__))
        
        # Path to compiled C++ executable (simulator)
        self.cpp_executable = None
        self.try_find_executable()
    
    def try_find_executable(self):
        # Try to find the executable in common locations
        possible_names = ["simulator", "myRISCVSim", "riscvsim", "a.out"]
        possible_exts = ["", ".exe"]
        
        search_paths = [
            self.base_path,
            os.path.join(self.base_path, ".."),
            os.path.join(self.base_path, "build"),
            os.path.join(self.base_path, "..", "build")
        ]
        
        for path in search_paths:
            for name in possible_names:
                for ext in possible_exts:
                    executable = os.path.join(path, name + ext)
                    if os.path.isfile(executable) and os.access(executable, os.X_OK):
                        self.cpp_executable = executable
                        self.status_bar.config(text=f"Using simulator: {os.path.basename(executable)}")
                        return
    
    def create_menu(self):
        menubar = tk.Menu(self.root)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Open", command=self.open_file)
        file_menu.add_command(label="Save Output", command=self.save_output)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        menubar.add_cascade(label="File", menu=file_menu)
        
        # About menu
        menubar.add_command(label="About", command=self.show_about)
        
        self.root.config(menu=menubar)
    
    def create_memory_tab(self):
        memory_frame = tk.Frame(left_panel)
        left_panel.add(memory_frame, text="Memory")

        search_bar = tk.Entry(memory_frame)
        search_bar.pack(fill=tk.X, padx=5, pady=5)

        self.memory_text = scrolledtext.ScrolledText(memory_frame, wrap=tk.WORD, font=("Courier", 10))
        self.memory_text.pack(fill=tk.BOTH, expand=True)

        search_bar.bind("<Return>", self.search_memory)

    def search_memory(self):
        query = self.search_bar.get()
        content = self.memory_text.get(1.0, tk.END)
    
    # Highlight matching lines
        self.memory_text.tag_remove("highlight", "1.0", tk.END)
        for line_num, line in enumerate(content.splitlines(), start=1):
            if query in line:
                start_idx = f"{line_num}.0"
                end_idx = f"{line_num}.{len(line)}"
                self.memory_text.tag_add("highlight", start_idx, end_idx)
                self.memory_text.tag_config("highlight", background="yellow")

    
    def create_toolbar(self):
        toolbar_frame = tk.Frame(self.root, bg="#e0e0e0")
        toolbar_frame.pack(fill=tk.X, padx=5, pady=2)

        style = ttk.Style()
        style.configure("TButton", font=("Arial", 12), padding=5)

        reset_btn = ttk.Button(toolbar_frame, text="🔄 Reset", command=self.reset_simulation)
        reset_btn.pack(side=tk.LEFT, padx=5)
        reset_btn.bind("<Enter>", lambda e: reset_btn.configure(style="Hover.TButton"))
        reset_btn.bind("<Leave>", lambda e: reset_btn.configure(style="TButton"))

        load_btn = ttk.Button(toolbar_frame, text="📂 Load Program", command=self.load_program)
        load_btn.pack(side=tk.LEFT, padx=5)
    
        run_btn = ttk.Button(toolbar_frame, text="▶️ Run", command=self.run_simulation)
        run_btn.pack(side=tk.LEFT, padx=5)

       
        
    def create_main_area(self):
        # Main container
        main_frame = tk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create paned window for resizable sections
        paned_window = ttk.PanedWindow(main_frame, orient=tk.HORIZONTAL)
        paned_window.pack(fill=tk.BOTH, expand=True)
        
        # Left panel - Program and Memory
        left_panel = ttk.Notebook(paned_window)
        paned_window.add(left_panel, weight=1)
        
        # Program tab
        program_frame = tk.Frame(left_panel)
        left_panel.add(program_frame, text="Program")
        
        self.program_text = scrolledtext.ScrolledText(program_frame, wrap=tk.WORD, font=("Courier", 10))
        self.program_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Memory tab
        memory_frame = tk.Frame(left_panel)
        left_panel.add(memory_frame, text="Memory")
        
        self.memory_text = scrolledtext.ScrolledText(memory_frame, wrap=tk.WORD, font=("Courier", 10))
        self.memory_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Right panel - Registers and Output
        right_panel = ttk.Notebook(paned_window)
        paned_window.add(right_panel, weight=1)
        
        # Registers tab
        registers_frame = tk.Frame(right_panel)
        right_panel.add(registers_frame, text="Registers")
        
        self.registers_text = scrolledtext.ScrolledText(registers_frame, wrap=tk.WORD, font=("Courier", 10))
        self.registers_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Output tab
        output_frame = tk.Frame(right_panel)
        right_panel.add(output_frame, text="Output")
        
        self.output_text = scrolledtext.ScrolledText(output_frame, wrap=tk.WORD, font=("Courier", 10))
        self.output_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
    def create_status_bar(self):
        self.status_bar = tk.Label(self.root, text="Ready", bd=1, relief=tk.SUNKEN, anchor=tk.W, bg="#f8f9fa", font=("Arial", 10))
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def update_status(self, message):
        self.status_bar.config(text=message)
        
    


    
    def configure_executable(self):
        executable_path = filedialog.askopenfilename(
            title="Select RISC-V Simulator Executable",
            filetypes=[("Executable files", "*.exe"), ("All files", "*.*")] if os.name == "nt" else [("All files", "*")]
        )
        
        if executable_path and os.path.isfile(executable_path):
            if os.access(executable_path, os.X_OK):
                self.cpp_executable = executable_path
                self.status_bar.config(text=f"Executable set: {os.path.basename(executable_path)}")
                messagebox.showinfo("Configuration", f"Simulator executable set to:\n{executable_path}")
            else:
                messagebox.showerror("Error", "Selected file is not executable.")
    
    def open_file(self):
        file_path = filedialog.askopenfilename(
            filetypes=[("Machine Code Files", "*.mc"), ("All Files", "*.*")]
        )
        if file_path:
            self.current_file = file_path
            try:
                with open(file_path, "r") as file:
                    content = file.read()
                    self.program_text.delete(1.0, tk.END)
                    self.program_text.insert(tk.END, content)
                self.status_bar.config(text=f"Opened: {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to open file: {str(e)}")
    
    def save_output(self):
        file_path = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")]
        )
        if file_path:
            try:
                with open(file_path, "w") as file:
                    file.write(self.output_text.get(1.0, tk.END))
                self.status_bar.config(text=f"Output saved to: {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save output: {str(e)}")
    
    def reset_simulation(self):

        try:
            # Create a modified main.cpp file to just reset the processor
            with open("temp_reset.cpp", "w") as f:
                f.write("""
                #include "myRISCVSim.cpp"
                
                int main() {
                    reset_proc();
                    return 0;
                }
                """)
            
            # Compile and run the temporary file
            compile_process = subprocess.Popen(["g++", "temp_reset.cpp", "-o", "temp_reset"],
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE)
            out, err = compile_process.communicate()
            
            if compile_process.returncode != 0:
                self.output_text.delete(1.0, tk.END)
                self.output_text.insert(tk.END, f"Compilation error:\n{err.decode()}")
                return
            
            # Run the compiled program
            run_process = subprocess.Popen(["./temp_reset"],
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)
            out, err = run_process.communicate()
            
            # Alternative approach: modify and run the main executable
            # process = subprocess.Popen([self.cpp_executable, "--reset"], 
            #                        stdout=subprocess.PIPE, 
            #                        stderr=subprocess.PIPE)
            # out, err = process.communicate()
            
            self.output_text.delete(1.0, tk.END)
            self.output_text.insert(tk.END, "Processor reset\n")
            
            if out:
                self.output_text.insert(tk.END, out.decode())
            if err:
                self.output_text.insert(tk.END, f"Errors:\n{err.decode()}\n")
            
            # Clear register and memory displays
            self.program_text.delete(1.0, tk.END)
            self.registers_text.delete(1.0, tk.END)
            self.memory_text.delete(1.0, tk.END)
            
            self.status_bar.config(text="Run new program")
            
            # Clean up temporary files
            try:
                os.remove("temp_reset.cpp")
                os.remove("temp_reset")
            except:
                pass
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to reset processor: {str(e)}")
    
    def load_program(self):
        file_path = filedialog.askopenfilename(
            filetypes=[("Machine Code Files", "*.mc"), ("All Files", "*.*")]
        )
        if file_path:
            self.current_file = file_path
            try:
                with open(file_path, "r") as file:
                    content = file.read()
                    self.program_text.delete(1.0, tk.END)
                    self.program_text.insert(tk.END, content)
                self.status_bar.config(text=f"Opened: {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to open file: {str(e)}")
        if not self.current_file:
            messagebox.showinfo("Info", "Please open a file first.")
            return
        
        try:
            # Create a modified main.cpp file to load the program
            with open("temp_load.cpp", "w") as f:
                f.write(f"""
                #include "myRISCVSim.cpp"
                
                int main() {{
                    reset_proc();
                    load_program_memory("{self.current_file}");
                    return 0;
                }}
                """)
            
            # Compile and run the temporary file
            compile_process = subprocess.Popen(["g++", "temp_load.cpp", "-o", "temp_load"],
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE)
            out, err = compile_process.communicate()
            
            if compile_process.returncode != 0:
                self.output_text.delete(1.0, tk.END)
                self.output_text.insert(tk.END, f"Compilation error:\n{err.decode()}")
                return
            
            # Run the compiled program
            run_process = subprocess.Popen(["./temp_load"],
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)
            out, err = run_process.communicate()
                
            self.output_text.delete(1.0, tk.END)
            self.output_text.insert(tk.END, f"Program loaded: {os.path.basename(self.current_file)}\n")
            
            if out:
                self.output_text.insert(tk.END, out.decode())
            if err:
                self.output_text.insert(tk.END, f"Errors:\n{err.decode()}\n")
            
            self.status_bar.config(text=f"Program loaded: {os.path.basename(self.current_file)}")
            
            # Clean up temporary files
            try:
                os.remove("temp_load.cpp")
                os.remove("temp_load")
            except:
                pass
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load program: {str(e)}")
    
    def run_simulation(self):
        if not self.current_file:
            messagebox.showinfo("Info", "Please load a program first.")
            return
        
        try:
            # Update status and output
            self.status_bar.config(text="Compiling and running main.cpp...")
            self.output_text.delete(1.0, tk.END)
            self.output_text.insert(tk.END, "Compiling and running main.cpp...\n")
            self.root.update_idletasks()
            
            # Compile main.cpp
            main_cpp_path = os.path.join(self.base_path, "main.cpp")
            if not os.path.exists(main_cpp_path):
                self.output_text.insert(tk.END, f"Error: main.cpp not found at {main_cpp_path}\n")
                return
                
            compile_process = subprocess.Popen(["g++", main_cpp_path, "-o", "main"],
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE)
            out, err = compile_process.communicate()
            
            if compile_process.returncode != 0:
                self.output_text.insert(tk.END, f"Compilation error:\n{err.decode()}")
                return
            
            # Run the compiled program with the current file as input
            run_process = subprocess.Popen(["./main", self.current_file],
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
            out, err = run_process.communicate()
            
            # Process output
            if out:
                self.output_text.insert(tk.END, out.decode())
            if err:
                self.output_text.insert(tk.END, f"Errors:\n{err.decode()}\n")
            
            self.output_text.insert(tk.END, "Simulation complete\n")
            self.status_bar.config(text="Simulation complete")
            
            # Refresh memory and register displays
            self.refresh_memory()
            self.refresh_registers()
                
        except Exception as e:
            messagebox.showerror("Error", f"Simulation error: {str(e)}")
    def refresh_memory(self):
        # Read memory.mc file
        try:
            if os.path.exists("memory.mc"):
                with open("memory.mc", "r") as f:
                    memory_content = f.read()
                
                self.memory_text.delete(1.0, tk.END)
                self.memory_text.insert(tk.END, "Memory Contents:\n")
                self.memory_text.insert(tk.END, "-----------------\n")
                self.memory_text.insert(tk.END, memory_content)
                
                self.status_bar.config(text="Memory data refreshed")
            else:
                self.memory_text.delete(1.0, tk.END)
                self.memory_text.insert(tk.END, "Memory file (memory.mc) not found")
        except Exception as e:
            self.memory_text.delete(1.0, tk.END)
            self.memory_text.insert(tk.END, f"Error loading memory data: {str(e)}")
    
    def refresh_registers(self):
        # Read registerFile.mc file
        try:
            if os.path.exists("registerFile.mc"):
                with open("registerFile.mc", "r") as f:
                    register_content = f.read()
                
                self.registers_text.delete(1.0, tk.END)
                self.registers_text.insert(tk.END, "Register Contents:\n")
                self.registers_text.insert(tk.END, "-----------------\n") 
                self.registers_text.insert(tk.END, register_content)
                
                self.status_bar.config(text="Register data refreshed")
            else:
                self.registers_text.delete(1.0, tk.END)
                self.registers_text.insert(tk.END, "Register file (registerFile.mc) not found")
        except Exception as e:
            self.registers_text.delete(1.0, tk.END)
            self.registers_text.insert(tk.END, f"Error loading register data: {str(e)}")
    
    def show_about(self):
        about_text = """RISC-V Simulator GUI

CS204: Computer Architecture
Project Phase 2: Functional Simulator for subset of RISCV Processor

Features:
- Load and run RISC-V programs
- View memory and register contents
- Interactive simulation control
"""
        messagebox.showinfo("About", about_text)

if __name__ == "__main__":
    root = tk.Tk()
    app = RISCVSimulatorGUI(root)
    root.mainloop()
