from flask import Flask, request, jsonify, render_template
import subprocess
import json

app = Flask(__name__)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/generate", methods=["POST"])
def generate():
    try:
        data = request.get_json()
        print("Received data:", data)  # Debugging log

        # Save input to input.json
        with open("input.json", "w") as f:
            json.dump(data, f, indent=4)
        print("Input saved to input.json")  # Debugging log

        # Prepare input for the C++ program
        cpp_input = f"{data['numSections']}\n"
        cpp_input += f"{len(data['courses'])}\n"
        for course in data['courses']:
            cpp_input += f"{course['name']} {course['frequency']} {course['duration']} {','.join(course['professors'])}\n"

        cpp_input += f"{len(data['combinedClasses'])}\n"
        for combined in data['combinedClasses']:
            cpp_input += f"{combined['name']} {combined['frequency']} {combined['duration']} {','.join(combined['professors'])}\n"

        print("Input to C++ program:\n", cpp_input)  # Debugging log

        # Run the C++ executable
        result = subprocess.run(
            ["timetable.exe"],  # Make sure timetable.exe is built and in the correct directory
            input=cpp_input,
            text=True,
            capture_output=True
        )

        print("C++ program output:", result.stdout)  # Debugging log
        print("C++ program error (if any):", result.stderr)  # Debugging log

        if result.returncode == 0:
            # Read the JSON output file
            with open("output.json", "r") as f:
                output = json.load(f)
            return jsonify(output)
        else:
            return jsonify({
                "error": f"Timetable generation failed:\n{result.stderr}"
            })

    except Exception as e:
        print("Error in backend:", str(e))  # Debugging log
        return jsonify({
            "error": f"Server error: {str(e)}"
        })

if __name__ == "__main__":
    app.run(debug=True)
