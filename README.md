# WIGMS — Wedding & Invitation Guest Management System

> **Course:** Introduction to Programming / Mechatronics  
> **Institution:** PK Fokam Institute of Excellence  
> **Language:** C (console modules) + C with GTK 4 (graphical modules)  
> **Platform:** Windows via MSYS2 / MinGW-w64 (UCRT64)  
> **Version:** 4.0

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Repository Structure](#2-repository-structure)
3. [Prerequisites](#3-prerequisites)
4. [Installing MSYS2 and GTK 4](#4-installing-msys2-and-gtk-4)
5. [Cloning the Repository](#5-cloning-the-repository)
6. [File Descriptions](#6-file-descriptions)
7. [Using the Makefile](#7-using-the-makefile)
8. [Running the Programs](#8-running-the-programs)
9. [Data Storage (CSV Files)](#9-data-storage-csv-files)
10. [Access Codes (Admin Protection)](#10-access-codes-admin-protection)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. Project Overview

WIGMS is a guest management system designed for weddings and invitation events. It allows an organiser to register guests, group them into seating categories, track parking needs, and view event statistics through an admin dashboard.

The system is split into two layers:

| Layer | Description |
|---|---|
| **Console (C)** | Classic modular C programs that run directly in the terminal |
| **GUI (GTK 4)** | Three graphical applications with tabbed windows, dropdowns, and password-protected admin sections |

All data is saved automatically to CSV files (`persons.csv` and `categories.csv`) that sit next to the compiled executables.

---

## 2. Repository Structure

The repository is organised into **six subfolders** plus three root-level files. Each folder groups files by their role in the project.

```
C-PROGRAMMING-PROJECT-REPOSITORY/
│
├── Category/                   ← Console Category module
│   ├── category.c              ← Category functions implementation (linked list)
│   └── Category.h              ← Category function prototypes header
│
├── Data/                       ← CSV data storage
│   ├── persons.csv             ← Guest records (auto-created on first save)
│   └── categories.csv          ← Category records (auto-created on first save)
│
├── GUI/                        ← GTK 4 graphical applications
│   ├── wigms_home.c            ← Home Launcher + Admin Dashboard
│   ├── person_gtk.c            ← Guest Management GUI
│   └── category_gtk.c          ← Category Management GUI
│
├── Person/                     ← Console Person module
│   ├── Person.h                ← Shared data types (Person, Category, Gift structs)
│   └── Person_.c               ← Person console module (has its own main())
│
├── Program/                    ← Compiled executables
│   ├── wigms_home.exe          ← Compiled GTK 4 Home Launcher
│   ├── person_gtk.exe          ← Compiled GTK 4 Guest Manager
│   ├── category_gtk.exe        ← Compiled GTK 4 Category Manager
│   ├── person.exe              ← Compiled console Person module
│   └── category.exe            ← Compiled console Category module
│
├── Report/                     ← Project documentation
│   └── wigms_report.pdf        ← Full project report (PDF)
│
├── .gitignore                  ← Git ignored file patterns
├── LICENSE                     ← Project licence
├── README.md                   ← This file
└── makefile.txt                ← Makefile (rename to Makefile before use)
```

> 💡 **Note on the Makefile:** The file is stored as `makefile.txt` in the repository root. Before compiling, rename it to `Makefile` (no extension) in the UCRT64 terminal:
> ```bash
> mv makefile.txt Makefile
> ```

---

## 3. Prerequisites

Before compiling anything you need the following installed on your Windows machine:

| Tool | Purpose | Where to get it |
|---|---|---|
| **MSYS2** | Unix-like build environment for Windows | https://www.msys2.org |
| **MinGW-w64 GCC** | C compiler (installed via MSYS2) | Installed inside MSYS2 |
| **GTK 4** | GUI toolkit for the graphical apps | Installed via MSYS2 |
| **pkg-config** | Auto-detects GTK compiler flags | Installed via MSYS2 |
| **make** | Runs the Makefile build system | Installed via MSYS2 |
| **Git** | Cloning the repository | https://git-scm.com |

> ⚠️ **Important:** Always use the **MSYS2 UCRT64** terminal, not the plain MSYS2 or MinGW32 one. UCRT64 targets the modern Windows runtime and is required for GTK 4.

---

## 4. Installing MSYS2 and GTK 4

### Step 1 — Download and install MSYS2

Go to https://www.msys2.org and download the installer. Run it and install to the default path `C:\msys64`.

### Step 2 — Open the UCRT64 terminal

From the Windows Start menu, search for **MSYS2 UCRT64** and open it.

### Step 3 — Update the package database

```bash
pacman -Syu
```

If the terminal closes itself, reopen it and run:

```bash
pacman -Su
```

### Step 4 — Install all required packages

Paste this single command into the UCRT64 terminal:

```bash
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-pkg-config \
          make \
          git
```

Press `Y` when prompted. This will install approximately 500 MB of packages including GTK 4, GCC, and all their dependencies.

### Step 5 — Add MSYS2 to your Windows PATH

This allows you to run `gcc`, `make`, and `pkg-config` from any terminal window:

1. Press `Win + S` and search **"environment variables"**
2. Click **Edit the system environment variables**
3. Click **Environment Variables**
4. Under **System variables**, find **Path** → click **Edit**
5. Click **New** and paste: `C:\msys64\ucrt64\bin`
6. Click **OK** on all dialogs
7. Restart any open terminal windows

### Step 6 — Verify the installation

In the UCRT64 terminal, run:

```bash
gcc --version
pkg-config --modversion gtk4
make --version
```

All three commands should print version numbers without errors. GTK 4 should show version `4.x.x`.

---

## 5. Cloning the Repository

In the UCRT64 terminal, navigate to where you want to save the project, then clone it:

```bash
cd /c/Users/YourName
git clone https://github.com/junking237-prog/C-PROGRAMMING-PROJECT-REPOSITORY.git
cd C-PROGRAMMING-PROJECT-REPOSITORY
```

> 💡 **Tip:** In MSYS2, Windows paths use forward slashes and the drive letter becomes `/c/`, `/d/`, etc.  
> Example: `C:\Users\John\WIGMS` becomes `/c/Users/John/WIGMS`

To confirm you are in the right folder:

```bash
ls
```

You should see the six subfolders: `Category/`, `Data/`, `GUI/`, `Person/`, `Program/`, `Report/`, plus `README.md`, `LICENSE`, `.gitignore`, and `makefile.txt`.

### Step after cloning — prepare the Makefile and source files

Because the sources are spread across subfolders, you need to copy the files into a single working folder before compiling. Run these commands once after cloning:

```bash
# Create a flat working build folder
mkdir -p ~/wigms_build
cd ~/wigms_build

# Copy all source files into the build folder
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/Person/Person.h       .
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/Person/Person_.c      .
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/Category/category.c   ./Category.c
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/Category/Category.h   .
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/GUI/person_gtk.c      .
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/GUI/category_gtk.c    .
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/GUI/wigms_home.c      .

# Copy and rename the Makefile
cp ~/C-PROGRAMMING-PROJECT-REPOSITORY/makefile.txt          ./Makefile

# Verify everything is in place
ls
```

You should now see all `.c` and `.h` files plus the `Makefile` in one folder. You are ready to compile.

---

## 6. File Descriptions

### 📁 `Person/` folder — Console Person module

| File | Extension | Description |
|---|---|---|
| `Person.h` | `.h` (Header) | Declares the `side` enum and the `Person`, `Category`, `Gift` structs shared by all modules. Also acts as `Type.h` |
| `Person_.c` | `.c` (C Source) | Implements `create_person()` and `display_Person()` with a console menu. Has its own `main()` for standalone testing |

### 📁 `Category/` folder — Console Category module

| File | Extension | Description |
|---|---|---|
| `Category.h` | `.h` (Header) | Declares all Category function prototypes: `create_category`, `insert_category`, `delete_category`, `update_category`, `count_guest`, `display_all_guests` |
| `category.c` | `.c` (C Source) | Implements all Category functions using a singly linked list. Has its own `main()` with a text menu |

### 📁 `GUI/` folder — GTK 4 graphical applications

| File | Extension | Description |
|---|---|---|
| `person_gtk.c` | `.c` (C Source) | Full GTK 4 Guest Management window. Tabs: Register, Update, Delete, Search, All Guests (admin-protected) |
| `category_gtk.c` | `.c` (C Source) | Full GTK 4 Category Management window. Tabs: Register, Update, Delete, Categories, All Categories (admin), Classify By, Prioritize |
| `wigms_home.c` | `.c` (C Source) | GTK 4 Home Launcher. Tabs: Home (module cards) and Dashboard (admin-protected) |

### 📁 `Program/` folder — Compiled executables

| File | Extension | Description |
|---|---|---|
| `person.exe` | `.exe` | Compiled console Person module |
| `category.exe` | `.exe` | Compiled console Category module |
| `person_gtk.exe` | `.exe` | Compiled GTK 4 Guest Management app |
| `category_gtk.exe` | `.exe` | Compiled GTK 4 Category Management app |
| `wigms_home.exe` | `.exe` | Compiled GTK 4 Home Launcher |

### 📁 `Data/` folder — CSV data files

| File | Extension | Description |
|---|---|---|
| `persons.csv` | `.csv` | Guest records — created automatically on first save |
| `categories.csv` | `.csv` | Category records — created automatically on first save |

### 📁 `Report/` folder — Project documentation

| File | Extension | Description |
|---|---|---|
| `wigms_report.pdf` | `.pdf` | Full project report including UML diagrams, source code documentation, and GUI screenshots |

### Root-level files

| File | Description |
|---|---|
| `makefile.txt` | Build system for all modules — **rename to `Makefile`** before use |
| `README.md` | This documentation file |
| `LICENSE` | Project open-source licence |
| `.gitignore` | Tells Git which files to exclude from version control (e.g. `.exe`, `.o`) |

### Object file (auto-generated during build)

| File | Extension | Description |
|---|---|---|
| `Category.o` | `.o` (Object) | Intermediate object file produced during the modular Category compilation — not committed to Git |

---

## 7. Using the Makefile

All build commands are run from the UCRT64 terminal **inside the `wigms_build` folder** you created after cloning (see Section 5).

### Build everything at once

```bash
make
```

This compiles all five executables: `person.exe`, `category.exe`, `person_gtk.exe`, `category_gtk.exe`, and `wigms_home.exe`.

---

### Build individual targets

| Command | What it compiles | Output file |
|---|---|---|
| `make person` | Console Person module | `person.exe` |
| `make category` | Console Category module (modular) | `category.exe` |
| `make person_gtk` | GTK 4 Guest Management GUI | `person_gtk.exe` |
| `make category_gtk` | GTK 4 Category Management GUI | `category_gtk.exe` |
| `make wigms_home` | GTK 4 Home Launcher + Dashboard | `wigms_home.exe` |

---

### Build by group

| Command | What it does |
|---|---|
| `make console` | Builds only `person.exe` and `category.exe` |
| `make gui` | Builds only the three GTK 4 executables |

---

### Build and run immediately

| Command | Effect |
|---|---|
| `make run_person` | Compiles (if needed) then launches `person.exe` in the terminal |
| `make run_category` | Compiles (if needed) then launches `category.exe` in the terminal |
| `make run_person_gtk` | Compiles (if needed) then launches the GTK 4 Guest Manager window |
| `make run_category_gtk` | Compiles (if needed) then launches the GTK 4 Category Manager window |
| `make run_wigms_home` | Compiles (if needed) then launches the GTK 4 Home window |

---

### Utility commands

| Command | Effect |
|---|---|
| `make clean` | Deletes all `.exe` and `.o` compiled files |
| `make help` | Prints a full usage summary in the terminal |

---

### How the Category module is compiled (modular build explained)

The Category module is built in two stages to demonstrate proper modular C programming:

```
Stage 1:  Category.c  →  Category.o       (compile: gcc -c)
Stage 2:  Category.o  →  category.exe     (link:    gcc -o)
```

The Makefile tracks all dependencies automatically. If only `Category.c` changes, only that file is recompiled. If `Category.h` or `Type.h` changes, the object file is also rebuilt. This is the standard approach for large modular C projects.

The exact compiler commands the Makefile runs are:

```bash
# Stage 1 — compile to object file
gcc -Wall -Wextra -std=c11 -c -o Category.o Category.c

# Stage 2 — link to executable
gcc -Wall -Wextra -std=c11 -o category.exe Category.o
```

For the GTK 4 apps, `pkg-config` automatically provides the correct flags:

```bash
gcc -Wall -Wextra -std=c11 $(pkg-config --cflags gtk4) \
    -o person_gtk.exe person_gtk.c \
    $(pkg-config --libs gtk4)
```

---

## 8. Running the Programs

### Recommended entry point

Always start with the Home Launcher. It gives access to both GUI modules from one window:

```bash
./wigms_home.exe
```

From there, click **Open Guest Manager** to launch `person_gtk.exe`, or click **Open Category Manager** (admin password required) to launch `category_gtk.exe`.

---

### Running each program individually

#### Console Person module

```bash
./person.exe
```

A text menu appears in the terminal. Options:
- `1` — Register a new person (enter ID, name, age, social class, side)
- `2` — Display the last registered person
- `0` — Exit

#### Console Category module

```bash
./category.exe
```

A text menu appears. Options:
- `1` — Insert a new category
- `2` — Display all categories and their guests
- `3` — Update a category by ID
- `4` — Delete a category by ID
- `5` — Show total guest slot count
- `6` — Exit

#### GTK 4 Guest Management

```bash
./person_gtk.exe
```

A tabbed window opens with five tabs:

| Tab | Function |
|---|---|
| 📝 Register | Add a new guest (Name, Age, Social Class, Side, Parking). ID is auto-generated invisibly |
| ✏️ Update | Select a guest by name from a dropdown, load their data, edit, and save |
| 🗑 Delete | Select a guest by name and remove them permanently |
| 🔍 Search | Real-time search by name or social class. No ID column shown |
| 🔒 All Guests | **Admin only** — enter access code to see full list including internal IDs |

#### GTK 4 Category Management

```bash
./category_gtk.exe
```

> ⚠️ On the Home page, this app requires the admin password before it launches.

A tabbed window opens with seven tabs:

| Tab | Function |
|---|---|
| 📝 Register | Create a category with a code and assign up to 4 guests from a dropdown of registered persons |
| ✏️ Update | Load an existing category by code, modify it, and save |
| 🗑 Delete | Remove a category by selecting its code from a dropdown |
| 📋 Categories | Searchable list of all categories (no internal ID shown) |
| 🔒 All Categories | **Admin only** — full list with internal auto-generated IDs |
| 📊 Classify By | Groups all assigned guests by Name (A–Z), Age, or Social Class using a dropdown selector |
| ⚙️ Prioritize | **Merge sort** — sort guests by Age or Social Class, or sort all categories by code alphabetically. Click the **Sort** button to apply |

#### GTK 4 Home Launcher

```bash
./wigms_home.exe
```

A two-tab window:

| Tab | Function |
|---|---|
| 🏠 Home | Two cards — Guest Manager (public) and Category Manager (admin password required to launch) |
| 🔒 Dashboard | **Admin only** — event statistics: total guests, categories, LE/LA split, parking count, assigned vs unassigned guests, and a category guest breakdown table |

---

## 9. Data Storage (CSV Files)

All data is stored as plain CSV files in the **same folder** as the executables.

### `persons.csv`

Created automatically the first time a guest is saved.

```
id,name,age,social_class,side,parking
1,"Marie Claire",25,"VIP",LA,YES
2,"Jean Paul",30,"Family",LE,NO
```

| Column | Type | Description |
|---|---|---|
| `id` | integer | Auto-generated unique identifier (hidden from users) |
| `name` | string | Full guest name (must be unique) |
| `age` | integer | Guest age |
| `social_class` | string | e.g. VIP, Family, Friend, Colleague |
| `side` | LE or LA | Which side of the wedding (LE = groom, LA = bride) |
| `parking` | YES or NO | Whether the guest needs a parking spot |

### `categories.csv`

Created automatically the first time a category is saved.

```
id,code,g0_name,g1_name,g2_name,g3_name
1,"VIP-A","Marie Claire","Jean Paul","","Thomas"
2,"Family","Jesus","Jason","","—"
```

| Column | Type | Description |
|---|---|---|
| `id` | integer | Auto-generated unique identifier (admin-only) |
| `code` | string | Category label e.g. VIP-A, Family, Friends |
| `g0_name`–`g3_name` | string | Names of up to 4 assigned guests (empty if slot unused) |

> 💡 Both CSV files can be opened in Excel or a text editor if needed. Do not add extra columns or change the header row — this will break the application.

---

## 10. Access Codes (Admin Protection)

Several sections of the application are restricted to the administrator. The access codes are entered as a password (characters shown as `●●●●`).

| Protected area | Application | Access code |
|---|---|---|
| **Open Category Manager** button | `wigms_home.exe` Home tab | `pokemon` |
| **🔒 Dashboard** tab | `wigms_home.exe` | `pokemon 1` |
| **🔒 All Guests** tab | `person_gtk.exe` | `pokemon` |
| **🔒 All Categories** tab | `category_gtk.exe` | `pokemon` |

> ⚠️ These codes are defined as constants at the top of each source file (`#define ADMIN_CODE "pokemon"`). To change a code, edit the constant and recompile with `make`.

---

## 11. Troubleshooting

### `pkg-config: command not found`

GTK 4 or pkg-config is not installed. Run:
```bash
pacman -S mingw-w64-ucrt-x86_64-pkg-config mingw-w64-ucrt-x86_64-gtk4
```

### `gcc: command not found`

GCC is not in your PATH. Make sure `C:\msys64\ucrt64\bin` is in your Windows system PATH variable (see [Step 5](#step-5--add-msys2-to-your-windows-path)).

### `make: command not found`

Install make inside MSYS2:
```bash
pacman -S make
```

### `No such file or directory: persons.csv`

This is not an error. The CSV files are created automatically the first time you save a guest or category. Simply run the app and register one record.

### The GTK window opens but closes instantly

Run the app from the terminal (not by double-clicking the `.exe`). The terminal shows any error messages that explain what went wrong.

### `fatal error: wigms_gtk.c: No such file or directory`

You are not in the correct build folder. Navigate to the working folder first:
```bash
cd ~/wigms_build
ls
```
You should see all `.c` source files and the `Makefile` listed together.

### Category Manager launches but cannot find persons

Both apps read from the **same folder**. Make sure `persons.csv` and `categories.csv` are in the same directory as all three `.exe` files. All five executables must live together:

```
WIGMS/
  wigms_home.exe
  person_gtk.exe
  category_gtk.exe
  persons.csv
  categories.csv
```

### Changes made in one GTK app don't appear in another

Click the **🔄 Refresh** button on the Dashboard, or close and reopen the app. Each app reads the CSV files fresh on every operation, but the Dashboard requires a manual refresh since it is a separate process.

---

## Compile Reference Card

Quick copy-paste commands for the UCRT64 terminal:

```bash
# Full build
make

# Individual GTK apps
gcc -Wall -Wextra -std=c11 $(pkg-config --cflags gtk4) -o person_gtk.exe   person_gtk.c   $(pkg-config --libs gtk4)
gcc -Wall -Wextra -std=c11 $(pkg-config --cflags gtk4) -o category_gtk.exe category_gtk.c $(pkg-config --libs gtk4)
gcc -Wall -Wextra -std=c11 $(pkg-config --cflags gtk4) -o wigms_home.exe   wigms_home.c   $(pkg-config --libs gtk4)

# Console modules
gcc -Wall -Wextra -std=c11 -o person.exe   Person_.c
gcc -Wall -Wextra -std=c11 -c -o Category.o Category.c
gcc -Wall -Wextra -std=c11 -o category.exe Category.o

# Run
./wigms_home.exe        # Start here — recommended entry point
./person_gtk.exe        # Guest management only
./category_gtk.exe      # Category management only (admin password needed from Home)
./person.exe            # Console person module
./category.exe          # Console category module

# Clean
make clean
```

---

*WIGMS v4.0 — PK Fokam Institute of Excellence*
