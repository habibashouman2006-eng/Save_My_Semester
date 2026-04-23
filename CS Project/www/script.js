const messageBox = document.getElementById('messageBox');
const taskTableBody = document.getElementById('taskTableBody');
const statusCards = document.getElementById('statusCards');

async function apiFetch(path, options = {}) {
    const res = await fetch(path, options);
    if (!res.ok) throw new Error(`Request failed: ${res.status}`);
    return res.json();
}

function showMessage(text, type = 'info') {
    const icon = type === 'error' ? '⚠️' : type === 'success' ? '✅' : 'ℹ️';
    messageBox.innerHTML = `<strong>${icon}</strong> ${text}`;
}

function formatUrgency(score) {
    if (score >= 8) return '<span class="status-chip high">Critical</span>';
    if (score >= 5) return '<span class="status-chip moderate">High</span>';
    return '<span class="status-chip low">Low</span>';
}

function buildTaskRow(task, index) {
    const remaining = (task.durationHours * (1.0 - task.progress / 100.0)).toFixed(1);
    return `
        <tr>
            <td>${index + 1}</td>
            <td>${task.name}</td>
            <td>${task.subject}</td>
            <td>${task.deadline}</td>
            <td>${task.durationHours.toFixed(1)}</td>
            <td>${task.difficulty}</td>
            <td>${task.gradeWeight.toFixed(0)}%</td>
            <td>${task.progress.toFixed(0)}%</td>
            <td>${task.priorityScore.toFixed(1)}</td>
            <td>
                <button onclick="completeTask(${task.id})">Complete</button>
                <button onclick="promptProgress(${task.id}, ${task.progress})">Update</button>
                <button onclick="deleteTask(${task.id})" title="Delete task">❌</button>
            </td>
        </tr>
    `;
}

async function loadTasks() {
    try {
        const tasks = await apiFetch('/api/tasks');
        const activeTasks = tasks
            .filter(task => !task.completed)
            .sort((a, b) => b.priorityScore - a.priorityScore);
        taskTableBody.innerHTML = activeTasks.map((task, index) => buildTaskRow(task, index)).join('');
        const openCount = activeTasks.length;
        const overdueCount = activeTasks.filter(task => task.daysUntilDeadline < 0).length;
        const completedCount = tasks.filter(task => task.completed).length;
        statusCards.innerHTML = `
            <div class="card"><strong>${openCount}</strong><span>Active tasks</span></div>
            <div class="card"><strong>${overdueCount}</strong><span>Overdue tasks</span></div>
            <div class="card"><strong>${completedCount}</strong><span>Completed</span><button onclick="resetCompleted()" title="Reset completed tasks">🔄</button></div>
        `;
        showMessage(openCount === 0 ? 'No active tasks yet. Add your first assignment.' : 'Task list loaded successfully.', 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function addTask(event) {
    event.preventDefault();
    try {
        const payload = {
            name: document.getElementById('nameInput').value,
            subject: document.getElementById('subjectInput').value,
            deadline: document.getElementById('deadlineInput').value,
            durationHours: parseFloat(document.getElementById('hoursInput').value),
            difficulty: parseInt(document.getElementById('difficultyInput').value, 10),
            gradeWeight: parseFloat(document.getElementById('weightInput').value),
            progress: parseFloat(document.getElementById('progressInput').value)
        };
        await apiFetch('/api/add', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        document.getElementById('taskForm').reset();
        await loadTasks();
        showMessage('New assignment added.', 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function completeTask(id) {
    try {
        await apiFetch('/api/update', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, completed: true })
        });
        await loadTasks();
        showMessage('Task marked complete.', 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function promptProgress(id, currentProgress) {
    const input = prompt('Enter new progress percentage (0-100):', currentProgress);
    if (input === null) return;
    const value = Number(input);
    if (Number.isNaN(value) || value < 0 || value > 100) {
        showMessage('Progress must be between 0 and 100.', 'error');
        return;
    }
    try {
        await apiFetch('/api/update', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id, progress: value })
        });
        await loadTasks();
        showMessage('Progress updated.', 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function fetchSummary(path, label) {
    try {
        const result = await apiFetch(path);
        if (path === '/api/overload') {
            showMessage(result.explanation, 'info');
        } else if (path === '/api/small-wins') {
            showMessage(`You have ${result.wins.length} small wins. ${result.explanation}`, 'info');
        } else if (path === '/api/burnout') {
            showMessage(`Stress level: ${result.stressPercent.toFixed(0)}%. ${result.explanation}`, result.stressPercent > 50 ? 'error' : 'success');
        }
        document.getElementById('messagePanel').scrollIntoView({ behavior: 'smooth' });
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function rescheduleOverdue() {
    try {
        const result = await apiFetch('/api/reschedule');
        await loadTasks();
        showMessage(result.message, 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function resetCompleted() {
    try {
        const result = await apiFetch('/api/reset-completed', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: '{}'});
        await loadTasks();
        showMessage(result.message, 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

async function deleteTask(id) {
    if (!confirm('Are you sure you want to delete this task?')) return;
    try {
        const result = await apiFetch('/api/delete', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({id})});
        await loadTasks();
        showMessage(result.message, 'success');
    } catch (error) {
        showMessage(error.message, 'error');
    }
}

document.getElementById('taskForm').addEventListener('submit', addTask);
document.getElementById('refreshButton').addEventListener('click', loadTasks);
document.getElementById('overloadButton').addEventListener('click', () => fetchSummary('/api/overload', 'Overload'));
document.getElementById('smallWinsButton').addEventListener('click', () => fetchSummary('/api/small-wins', 'Small Wins'));
document.getElementById('burnoutButton').addEventListener('click', () => fetchSummary('/api/burnout', 'Burnout'));
document.getElementById('rescheduleButton').addEventListener('click', rescheduleOverdue);

window.addEventListener('DOMContentLoaded', loadTasks);
