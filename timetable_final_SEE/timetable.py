import random
import time

DAYS = 5
SLOTS_PER_DAY = 6

class Class:
    def __init__(self, name, frequency_per_week, duration, is_combined, professors):
        self.name = name
        self.frequency_per_week = frequency_per_week
        self.is_lab = duration == 2
        self.duration = duration
        self.is_combined = is_combined
        self.professors = professors

class Section:
    def __init__(self, name, semester):
        self.name = name
        self.semester = semester

sections = []
classes = {}  # keyed by semester
combined_classes = {}  # keyed by semester
timetable = {}  # keyed by section name
section_professors = {}  # keyed by section name and class name
professor_schedule = {}  # keyed by professor and day -> list of occupied slots
random.seed(time.time())

def can_fit(day, start, duration):
    return all(day[i] == "" for i in range(start, start + duration) if i < SLOTS_PER_DAY)

def is_class_scheduled_on_day(day, class_name):
    return any(class_name in slot for slot in day)

def is_professor_available(prof, day, start, duration):
    for i in range(start, start + duration):
        if i in professor_schedule.get(prof, {}).get(day, set()):
            return False
    return True

def occupy_professor(prof, day, start, duration):
    if prof not in professor_schedule:
        professor_schedule[prof] = {}
    if day not in professor_schedule[prof]:
        professor_schedule[prof][day] = set()
    for i in range(start, start + duration):
        professor_schedule[prof][day].add(i)

def select_random_professor(professors):
    return random.choice(professors)

def pre_assign_professors():
    # Initialize for each section
    for section in sections:
        semester = section.semester
        section_professors[section.name] = {}

    # Assign professors for individual classes
    for semester in classes:
        for c in classes[semester]:
            relevant_sections = [s for s in sections if s.semester == semester]
            shuffled_profs = c.professors.copy()
            random.shuffle(shuffled_profs)
            for i, sec in enumerate(relevant_sections):
                assigned_prof = shuffled_profs[i % len(shuffled_profs)]
                section_professors[sec.name][c.name] = assigned_prof

    # Assign professors for combined classes
    for semester in combined_classes:
        for c in combined_classes[semester]:
            relevant_sections = [s for s in sections if s.semester == semester]
            shuffled_profs = c.professors.copy()
            random.shuffle(shuffled_profs)
            for i, sec in enumerate(relevant_sections):
                assigned_prof = shuffled_profs[i % len(shuffled_profs)]
                section_professors[sec.name][c.name] = assigned_prof

def init_timetables():
    for section in sections:
        timetable[section.name] = [["" for _ in range(SLOTS_PER_DAY)] for _ in range(DAYS)]

def get_lab_slots():
    return [0, 2, 4]

def assign_combined_classes():
    for semester in combined_classes:
        for c in combined_classes[semester]:
            relevant_sections = [s for s in sections if s.semester == semester]
            for _ in range(c.frequency_per_week):
                placed = False
                for d in range(DAYS):
                    if placed: break
                    if is_class_scheduled_on_day(timetable[relevant_sections[0].name][d], c.name):
                        continue
                    slots_to_try = get_lab_slots() if c.duration == 2 else list(range(SLOTS_PER_DAY))
                    for t in slots_to_try:
                        if t + c.duration > SLOTS_PER_DAY:
                            continue
                        if all(can_fit(timetable[sec.name][d], t, c.duration) for sec in relevant_sections):
                            prof_valid = True
                            for sec in relevant_sections:
                                prof = section_professors[sec.name][c.name]
                                if not is_professor_available(prof, d, t, c.duration):
                                    prof_valid = False
                                    break
                            if not prof_valid:
                                continue
                            for sec in relevant_sections:
                                prof = section_professors[sec.name][c.name]
                                for k in range(c.duration):
                                    timetable[sec.name][d][t + k] = f"{c.name} ({prof})"
                                occupy_professor(prof, d, t, c.duration)
                            placed = True
                            break

def assign_individual_classes():
    for section in sections:
        semester = section.semester
        grid = timetable[section.name]
        section_classes = sorted(classes[semester], key=lambda x: (not x.is_lab, -x.frequency_per_week))
        for c in section_classes:
            for _ in range(c.frequency_per_week):
                placed = False
                day_order = random.sample(range(DAYS), DAYS)
                for day_idx in day_order:
                    if placed: break
                    if is_class_scheduled_on_day(grid[day_idx], c.name):
                        continue
                    slots_to_try = get_lab_slots() if c.duration == 2 else list(range(SLOTS_PER_DAY))
                    random.shuffle(slots_to_try)
                    for t in slots_to_try:
                        if can_fit(grid[day_idx], t, c.duration):
                            prof = section_professors[section.name][c.name]
                            if is_professor_available(prof, day_idx, t, c.duration):
                                for k in range(c.duration):
                                    grid[day_idx][t + k] = f"{c.name} ({prof})"
                                occupy_professor(prof, day_idx, t, c.duration)
                                placed = True
                                break

def load_input_dict(data):
    for sem_data in data["semesters"]:
        sem = sem_data["semester"]
        for i in range(sem_data["numSections"]):
            sections.append(Section(f"{sem}_Section_{i+1}", sem))

        classes[sem] = []
        for c in sem_data["courses"]:
            classes[sem].append(Class(c["name"], c["frequency"], c["duration"], False, c["professors"]))

        combined_classes[sem] = []
        for c in sem_data["combinedClasses"]:
            combined_classes[sem].append(Class(c["name"], c["frequency"], c["duration"], True, c["professors"]))

def get_output_dict():
    out = {}
    for section, grid in timetable.items():
        out[section] = [day for day in grid]
    return out
