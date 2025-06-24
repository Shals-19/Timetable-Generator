from flask import Flask, request, jsonify, render_template
from timetable import load_input_dict, init_timetables, pre_assign_professors, assign_combined_classes, assign_individual_classes, get_output_dict

app = Flask(__name__)

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/generate", methods=["POST"])
def generate():
    try:
        data = request.get_json()
        
        # Clear any existing state if needed (reset globals manually if required)
        load_input_dict(data)  # Loads semesters, sections, classes
        init_timetables()
        pre_assign_professors()
        assign_combined_classes()
        assign_individual_classes()
        output = get_output_dict()

        return jsonify(output)
    except Exception as e:
        return jsonify({"error": f"Server error: {str(e)}"})

if __name__ == "__main__":
    app.run(debug=True)
