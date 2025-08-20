#include "QsLog.h"
#include <QDateTime>
#include <QVector>
#include <QMutex>
#include <QThreadPool>
#include <QQueue>
#include <QDebug>
#include <QRunnable>
#include <QWaitCondition>
#include <atomic>
#include <stdexcept>
#include <QSharedPointer>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>

namespace QsLogging {

// 单例模式的日志器实例指针
static QsLogging::Logger* s_instance = nullptr;
// 保护单例实例的互斥锁
static QMutex s_instanceMutex;

// 要创建的 HTML 文件的文件名
const QString HTML_FILENAME = "sqlite_viewer.html";

const QString HTML_CONTENT = R"(
   <!DOCTYPE html>
   <html lang="zh">
   <head>
       <meta charset="UTF-8">
       <meta name="viewport" content="width=device-width, initial-scale=1.0">
       <title>SQLite 数据浏览器</title>
       <!-- 使用 Inter 字体，提供现代感的外观 -->
       <link rel="stylesheet" href="https://rsms.me/inter/inter.css">
       <!-- 引入 Tailwind CSS 进行快速、响应式布局 -->
       <script src="https://cdn.tailwindcss.com"></script>
       <style>
           body {
               font-family: 'Inter', sans-serif;
               background-color: #f3f4f6;
           }
           .container {
               max-width: 95%;
               padding: 2rem;
               margin: 2rem auto;
               background-color: white;
               border-radius: 1rem;
               box-shadow: 0 10px 15px rgba(0, 0, 0, 0.1);
           }
           .data-table {
               width: 100%;
               border-collapse: separate;
               border-spacing: 0;
               margin-top: 1.5rem;
           }
           .data-table th, .data-table td {
               border: 1px solid #e5e7eb;
               padding: 0.75rem;
               text-align: left;
               word-wrap: break-word;
               font-size: 0.875rem; /* text-sm */
           }
           .data-table th {
               background-color: #f9fafb;
               font-weight: 600;
               color: #111827;
               position: sticky;
               top: 0;
           }
           /* 添加交替行颜色以提高可读性 */
           .data-table tbody tr:nth-child(even) {
               background-color: #f9fafb;
           }
           .data-container {
               max-height: 70vh;
               overflow-y: auto;
               border: 1px solid #d1d5db;
               border-radius: 0.5rem;
               margin-top: 1.5rem;
           }
           .log-level-badge {
               display: inline-block;
               padding: 0.25rem 0.5rem;
               font-size: 0.75rem;
               line-height: 1;
               font-weight: 600;
               border-radius: 9999px;
               text-transform: uppercase;
           }
           .level-trace { background-color: #e0f2fe; color: #075985; }
           .level-debug { background-color: #f0f9ff; color: #0284c7; }
           .level-info { background-color: #dbeafe; color: #1e40af; }
           .level-warn { background-color: #fef3c7; color: #92400e; }
           .level-error { background-color: #fee2e2; color: #991b1b; }
           .level-fatal { background-color: #fecaca; color: #991b1b; font-weight: 700; }
           /* 模态框动画效果 */
           .modal {
               transition: opacity 0.3s ease-in-out, transform 0.3s ease-in-out;
               transform: scale(1.05);
           }
           .modal.hidden {
               opacity: 0;
               transform: scale(0.95);
           }
           .modal.visible {
               opacity: 1;
               transform: scale(1.0);
           }
           .modal-overlay {
               background-color: rgba(0, 0, 0, 0.5);
           }
           .animate-spin-fast {
               animation: spin 0.75s linear infinite;
           }
           @keyframes spin {
               from { transform: rotate(0deg); }
               to { transform: rotate(360deg); }
           }
       </style>
   </head>
   <body class="bg-gray-100 p-4">

       <!-- Main container -->
       <div class="container">
           <h1 class="text-3xl md:text-4xl font-extrabold text-center text-gray-900 mb-2">SQLite 数据浏览器</h1>
           <p class="text-center text-gray-600 mb-6">请上传您的 SQLite `.db` 文件来浏览数据。</p>

           <!-- File upload and action controls -->
           <div class="bg-gray-50 p-6 rounded-xl shadow-inner mb-6 flex flex-col md:flex-row md:items-end md:space-x-4 space-y-4 md:space-y-0">
               <!-- File input -->
               <div class="flex-grow">
                   <label for="dbFile" class="block text-sm font-medium text-gray-700 mb-1">选择数据库文件</label>
                   <input type="file" id="dbFile" accept=".db" class="block w-full text-sm text-gray-500
                       file:mr-4 file:py-2 file:px-4
                       file:rounded-full file:border-0
                       file:text-sm file:font-semibold
                       file:bg-blue-50 file:text-blue-700
                       hover:file:bg-blue-100 cursor-pointer"/>
               </div>
               <!-- Action buttons -->
               <div class="flex flex-col sm:flex-row space-y-2 sm:space-y-0 sm:space-x-2 w-full md:w-auto">
                   <button id="loadDbButton" class="bg-blue-600 text-white font-semibold py-2 px-6 rounded-full shadow-lg hover:bg-blue-700 transition duration-300 transform hover:scale-105">
                       加载并显示数据
                   </button>
                   <button id="saveDbButton" class="bg-green-600 text-white font-semibold py-2 px-6 rounded-full shadow-lg hover:bg-green-700 transition duration-300 transform hover:scale-105 hidden" disabled>
                       保存数据库
                   </button>
                   <button id="clearDataButton" class="bg-red-500 text-white font-semibold py-2 px-6 rounded-full shadow-lg hover:bg-red-600 transition duration-300 transform hover:scale-105 hidden" disabled>
                       清空显示
                   </button>
               </div>
           </div>

           <!-- Filter Controls -->
           <div id="filterControls" class="bg-white p-6 rounded-xl shadow mb-6 hidden">
               <h2 class="text-2xl font-bold text-gray-800 mb-4">数据筛选</h2>
               <div class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-5 gap-4">
                   <!-- Keyword search -->
                   <div>
                       <label for="keywordSearch" class="block text-sm font-medium text-gray-700">关键字搜索</label>
                       <input type="text" id="keywordSearch" placeholder="请输入关键字" class="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 sm:text-sm p-2">
                   </div>
                   <!-- Log Level -->
                   <div>
                       <label for="logLevelFilter" class="block text-sm font-medium text-gray-700">日志级别</label>
                       <select id="logLevelFilter" class="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 sm:text-sm p-2">
                           <option value="">所有级别</option>
                           <option value="0">TRACE</option>
                           <option value="1">DEBUG</option>
                           <option value="2">INFO</option>
                           <option value="3">WARN</option>
                           <option value="4">ERROR</option>
                           <option value="5">FATAL</option>
                       </select>
                   </div>
                   <!-- Start Time -->
                   <div>
                       <label for="startTime" class="block text-sm font-medium text-gray-700">起始时间</label>
                       <input type="datetime-local" id="startTime" class="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 sm:text-sm p-2">
                   </div>
                   <!-- End Time -->
                   <div>
                       <label for="endTime" class="block text-sm font-medium text-gray-700">结束时间</label>
                       <input type="datetime-local" id="endTime" class="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 sm:text-sm p-2">
                   </div>
                   <!-- Rows per page -->
                   <div>
                       <label for="rowsLimit" class="block text-sm font-medium text-gray-700">显示条数</label>
                       <input type="number" id="rowsLimit" value="5000" min="1" class="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 sm:text-sm p-2">
                   </div>
               </div>
           </div>

           <!-- Status message and loading indicator -->
           <div id="statusMessage" class="mt-4 text-center text-gray-700"></div>
           <div id="loadingIndicator" class="hidden mt-4 flex justify-center">
               <div class="animate-spin-fast rounded-full h-8 w-8 border-t-2 border-b-2 border-blue-500"></div>
           </div>

           <!-- Data display area -->
           <div id="dataDisplay" class="hidden data-container">
               <table id="dataTable" class="data-table">
                   <thead>
                       <tr></tr>
                   </thead>
                   <tbody></tbody>
               </table>
           </div>
       </div>

       <!-- Modal for editing -->
       <div id="editModal" class="modal fixed inset-0 flex items-center justify-center bg-gray-600 bg-opacity-50 z-50 hidden">
           <div class="relative w-full max-w-lg mx-4 p-5 border shadow-lg rounded-md bg-white">
               <h3 class="text-xl font-bold mb-4">修改行数据</h3>
               <form id="editForm" class="space-y-4 max-h-96 overflow-y-auto"></form>
               <div class="mt-4 flex justify-end space-x-2">
                   <button id="saveEdit" class="bg-blue-600 text-white font-semibold py-2 px-4 rounded-full hover:bg-blue-700 transition">保存</button>
                   <button id="cancelEdit" class="bg-gray-400 text-white font-semibold py-2 px-4 rounded-full hover:bg-gray-500 transition">取消</button>
               </div>
           </div>
       </div>

       <!-- Custom Message Box Modal -->
       <div id="messageModal" class="modal fixed inset-0 flex items-center justify-center bg-gray-600 bg-opacity-50 z-50 hidden">
           <div class="relative w-full max-w-sm mx-4 p-5 border shadow-lg rounded-md bg-white">
               <div id="messageContent" class="mb-4 text-center"></div>
               <div class="flex justify-end space-x-2">
                   <button id="messageConfirm" class="bg-blue-600 text-white font-semibold py-2 px-4 rounded-full hover:bg-blue-700 transition">确定</button>
               </div>
           </div>
       </div>

       <!-- Custom Confirm Modal -->
       <div id="confirmModal" class="modal fixed inset-0 flex items-center justify-center bg-gray-600 bg-opacity-50 z-50 hidden">
           <div class="relative w-full max-w-sm mx-4 p-5 border shadow-lg rounded-md bg-white">
               <div id="confirmContent" class="mb-4 text-center"></div>
               <div class="flex justify-end space-x-2">
                   <button id="confirmYes" class="bg-red-600 text-white font-semibold py-2 px-4 rounded-full hover:bg-red-700 transition">是</button>
                   <button id="confirmNo" class="bg-gray-400 text-white font-semibold py-2 px-4 rounded-full hover:bg-gray-500 transition">否</button>
               </div>
           </div>
       </div>

       <!-- Include sql.js library -->
       <script src="https://cdnjs.cloudflare.com/ajax/libs/sql.js/1.8.0/sql-wasm.js"></script>
       <script>
           // Global variables to hold the full data and the filtered data
           let fullData = [];
           let columns = [];
           let levelColumnIndex = -1;
           let db = null;
           let tableName = '';
           let uniqueIdColumn = null;
           let originalFileName = 'data.db';

           // Map log levels from number to string and define color classes
           const levelMap = ['TRACE', 'DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL'];
           const levelClasses = {
               0: 'level-trace',
               1: 'level-debug',
               2: 'level-info',
               3: 'level-warn',
               4: 'level-error',
               5: 'level-fatal'
           };

           const loadDbButton = document.getElementById('loadDbButton');
           const fileInput = document.getElementById('dbFile');
           const statusMessage = document.getElementById('statusMessage');
           const loadingIndicator = document.getElementById('loadingIndicator');
           const dataDisplay = document.getElementById('dataDisplay');
           const filterControls = document.getElementById('filterControls');
           const dataTable = document.getElementById('dataTable');
           const saveDbButton = document.getElementById('saveDbButton');
           const clearDataButton = document.getElementById('clearDataButton');

           // Custom modal functions to replace `alert()` and `confirm()`
           function showMessage(message) {
               const modal = document.getElementById('messageModal');
               document.getElementById('messageContent').textContent = message;
               modal.classList.remove('hidden');
               modal.classList.add('visible');
               document.getElementById('messageConfirm').onclick = () => {
                   modal.classList.add('hidden');
               };
           }

           function showConfirm(message, callback) {
               const modal = document.getElementById('confirmModal');
               document.getElementById('confirmContent').textContent = message;
               modal.classList.remove('hidden');
               modal.classList.add('visible');
               document.getElementById('confirmYes').onclick = () => {
                   modal.classList.add('hidden');
                   callback(true);
               };
               document.getElementById('confirmNo').onclick = () => {
                   modal.classList.add('hidden');
                   callback(false);
               };
           }

           // Add event listeners for filter controls
           document.getElementById('keywordSearch').addEventListener('input', filterAndRender);
           document.getElementById('logLevelFilter').addEventListener('change', filterAndRender);
           document.getElementById('startTime').addEventListener('change', filterAndRender);
           document.getElementById('endTime').addEventListener('change', filterAndRender);
           document.getElementById('rowsLimit').addEventListener('change', filterAndRender);

           // Listen for file selection change to enable/disable the load button
           fileInput.addEventListener('change', () => {
               loadDbButton.disabled = !fileInput.files.length;
           });

           clearDataButton.addEventListener('click', () => {
               renderTable([]);
               statusMessage.textContent = '已清空当前显示的数据。';
           });

           saveDbButton.addEventListener('click', () => {
               if (!db) {
                   showMessage('没有可保存的数据库。请先加载一个文件。');
                   return;
               }
               try {
                   // Export the database to a binary array
                   const binaryArray = db.export();
                   const blob = new Blob([binaryArray.buffer], { type: 'application/octet-stream' });
                   const a = document.createElement('a');
                   a.href = URL.createObjectURL(blob);
                   a.download = `edited_${originalFileName}`;
                   document.body.appendChild(a);
                   a.click();
                   document.body.removeChild(a);
                   showMessage('数据库已成功导出并准备下载。');
               } catch (error) {
                   console.error("保存数据库时发生错误:", error);
                   showMessage(`保存数据库时发生错误：${error.message}`);
               }
           });

           loadDbButton.addEventListener('click', async () => {
               const file = fileInput.files[0];
               if (!file) {
                   showMessage('请先选择一个数据库文件。');
                   return;
               }
               originalFileName = file.name;
               statusMessage.textContent = '正在加载数据库...';
               setUiState(true); // Disable UI and show loading

               try {
                   const fileReader = new FileReader();
                   fileReader.onload = async function() {
                       const arrayBuffer = this.result;
                       // Note: Character encoding is handled by the browser when reading the file.
                       // The sql.js library expects a binary array. If the data in the .db file itself is not UTF-8,
                       // garbled characters might appear, which cannot be fixed in the browser.
                       const SQL = await initSqlJs({ locateFile: file => `https://cdnjs.cloudflare.com/ajax/libs/sql.js/1.8.0/${file}` });
                       db = new SQL.Database(new Uint8Array(arrayBuffer));

                       // Get all table names
                       const tableNamesResult = db.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
                       const tableNames = tableNamesResult.length > 0 ? tableNamesResult[0].values.map(row => row[0]) : [];

                       if (tableNames.length === 0) {
                           statusMessage.textContent = '数据库中没有找到任何表。';
                           setUiState(false);
                           return;
                       }

                       // Assume the log table is the first one found or is named 'log'
                       tableName = tableNames.find(name => name.toLowerCase().includes('log')) || tableNames[0];

                       // Query all data
                       const queryResult = db.exec(`SELECT * FROM "${tableName}"`);

                       if (queryResult.length === 0 || !queryResult[0].columns || queryResult[0].values.length === 0) {
                           statusMessage.textContent = `表 "${tableName}" 中没有数据。`;
                           setUiState(false);
                           return;
                       }

                       columns = queryResult[0].columns;
                       fullData = queryResult[0].values;

                       // Identify a potential unique ID column
                       uniqueIdColumn = columns.find(col => ['id', 'rowid'].includes(col.toLowerCase()));

                       // Find the index of the 'level' column
                       levelColumnIndex = columns.findIndex(col => col.toLowerCase() === 'level');

                       // Show the filter and action controls after a successful load
                       filterControls.classList.remove('hidden');
                       clearDataButton.classList.remove('hidden');
                       saveDbButton.classList.remove('hidden');

                       statusMessage.textContent = `成功从表 "${tableName}" 加载了 ${fullData.length} 条数据。`;

                       dataDisplay.classList.remove('hidden');
                       setUiState(false); // Enable UI and hide loading

                       // Initial render of the table, showing the first N rows
                       filterAndRender();
                   };
                   fileReader.readAsArrayBuffer(file);
               } catch (error) {
                   console.error("加载数据库时发生错误:", error);
                   statusMessage.textContent = `加载数据库时发生错误：${error.message}`;
                   setUiState(false); // Enable UI and hide loading
               }
           });

           function setUiState(isLoading) {
               loadingIndicator.classList.toggle('hidden', !isLoading);
               loadDbButton.disabled = isLoading;
               fileInput.disabled = isLoading;
               saveDbButton.disabled = isLoading;
               clearDataButton.disabled = isLoading;
               document.getElementById('keywordSearch').disabled = isLoading;
               document.getElementById('logLevelFilter').disabled = isLoading;
               document.getElementById('startTime').disabled = isLoading;
               document.getElementById('endTime').disabled = isLoading;
               document.getElementById('rowsLimit').disabled = isLoading;
           }

           function filterAndRender() {
               const keyword = document.getElementById('keywordSearch').value.toLowerCase();
               const levelFilter = document.getElementById('logLevelFilter').value;
               const startTime = document.getElementById('startTime').value;
               const endTime = document.getElementById('endTime').value;
               const rowsLimit = parseInt(document.getElementById('rowsLimit').value, 10) || 5000;

               let filteredData = fullData.filter(row => {
                   // Keyword filter
                   const keywordMatch = row.some(cell => String(cell).toLowerCase().includes(keyword));
                   if (!keywordMatch) return false;

                   // Level filter
                   if (levelFilter && levelColumnIndex !== -1) {
                       if (String(row[levelColumnIndex]) !== levelFilter) {
                           return false;
                       }
                   }

                   // Time filter (requires a timestamp column)
                   const timestampColIndex = columns.findIndex(col => col.toLowerCase().includes('time') || col.toLowerCase().includes('date'));
                   if (timestampColIndex !== -1) {
                       const timestamp = new Date(row[timestampColIndex]);
                       if (startTime && new Date(startTime) > timestamp) return false;
                       if (endTime && new Date(endTime) < timestamp) return false;
                   }

                   return true;
               });

               // The data is no longer reversed here, displaying it in its original order.
               const limitedData = filteredData.slice(0, rowsLimit);

               renderTable(limitedData);
               statusMessage.textContent = `筛选后显示 ${limitedData.length} 条数据（总共 ${filteredData.length} 条）。`;
           }

           function renderTable(data) {
               const tableHead = dataTable.querySelector('thead tr');
               const tableBody = dataTable.querySelector('tbody');

               tableHead.innerHTML = '';
               tableBody.innerHTML = '';

               // Create table header, including the new '操作' (Actions) column
               const headerColumns = [...columns, '操作'];
               headerColumns.forEach(col => {
                   const th = document.createElement('th');
                   th.textContent = col;
                   tableHead.appendChild(th);
               });

               // Create table rows
               data.forEach((row, rowIndex) => {
                   const tr = document.createElement('tr');
                   row.forEach((cell, cellIndex) => {
                       const td = document.createElement('td');
                       if (cellIndex === levelColumnIndex) {
                           // Apply special styling for the 'level' column
                           const levelNum = parseInt(cell, 10);
                           const levelText = levelMap[levelNum] || 'UNKNOWN';
                           const levelClass = levelClasses[levelNum] || '';

                           const badge = document.createElement('span');
                           badge.textContent = levelText;
                           badge.classList.add('log-level-badge', levelClass);
                           td.appendChild(badge);
                       } else {
                           td.textContent = cell;
                       }
                       tr.appendChild(td);
                   });

                   // Add action buttons to the last cell
                   const actionsTd = document.createElement('td');
                   actionsTd.classList.add('flex', 'space-x-2');

                   // Edit button (using SVG for a clean look)
                   const editButton = document.createElement('button');
                   editButton.title = '编辑';
                   editButton.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" class="w-5 h-5 text-gray-500 hover:text-blue-600 transition duration-150"><path d="M21.731 2.269a2.625 2.625 0 00-3.712 0l-1.157 1.157 3.712 3.712 1.157-1.157a2.625 2.625 0 000-3.712zM19.513 8.125l-4.787-4.788-2.424 2.424 4.787 4.788 2.424-2.424zM20.375 6.5l-2.424-2.424-4.787 4.788-2.424-2.424 4.787-4.788-2.424-2.424zM3.75 12h-.75a.75.75 0 00-.75.75v.75A2.25 2.25 0 003.75 15h.75a.75.75 0 00.75-.75v-.75a2.25 2.25 0 00-2.25-2.25zM12 21a9 9 0 110-18 9 9 0 010 18z"></path></svg>`;
                   editButton.addEventListener('click', () => editRowInModal(row));
                   actionsTd.appendChild(editButton);

                   // Delete button (using SVG for a clean look)
                   const deleteButton = document.createElement('button');
                   deleteButton.title = '删除';
                   deleteButton.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" class="w-5 h-5 text-gray-500 hover:text-red-600 transition duration-150"><path fill-rule="evenodd" d="M16.5 4.478v.227a4.99 4.99 0 01-2.474 4.343l-1.745-1.745a.75.75 0 00-1.06 1.06l1.745 1.745a4.99 4.99 0 01-4.343 2.474H6.75a.75.75 0 00-.75.75v.75c0 .414.336.75.75.75h4.125a6.49 6.49 0 006.284-4.739.75.75 0 00-1.45-.395A4.99 4.99 0 0116.5 4.478zM19.5 3a.75.75 0 00-1.5 0v.75h-1.5a.75.75 0 000 1.5h1.5v1.5a.75.75 0 001.5 0v-1.5h1.5a.75.75 0 000-1.5h-1.5V3zM.75 12a.75.75 0 000 1.5h1.5v1.5a.75.75 0 001.5 0v-1.5h1.5a.75.75 0 000-1.5h-1.5v-1.5a.75.75 0 00-1.5 0v1.5H.75z" clip-rule="evenodd" /></svg>`;
                   deleteButton.addEventListener('click', () => deleteRowFromDb(row));
                   actionsTd.appendChild(deleteButton);

                   tr.appendChild(actionsTd);
                   tableBody.appendChild(tr);
               });
           }

           function deleteRowFromDb(row) {
               if (!db || !tableName) {
                   console.error("Database or table not loaded.");
                   return;
               }

               if (!uniqueIdColumn) {
                   showMessage('无法删除行，未找到唯一的ID列（如"id"）。');
                   return;
               }

               const uniqueId = row[columns.findIndex(col => col === uniqueIdColumn)];
               showConfirm(`确定要删除 ID 为 ${uniqueId} 的行吗？此操作是不可逆的。`, (confirmed) => {
                   if (confirmed) {
                       try {
                           const statement = db.prepare(`DELETE FROM "${tableName}" WHERE "${uniqueIdColumn}" = ?`);
                           statement.bind([uniqueId]);
                           statement.step();
                           statement.free();

                           // Remove the row from the in-memory data
                           fullData = fullData.filter(d => d[columns.findIndex(col => col === uniqueIdColumn)] !== uniqueId);

                           showMessage(`成功删除了 ID 为 ${uniqueId} 的行。请点击“保存数据库”下载更新后的文件。`);
                           filterAndRender();
                       } catch (e) {
                           console.error("删除行时出错:", e);
                           showMessage(`删除行时出错: ${e.message}`);
                       }
                   }
               });
           }

           function editRowInModal(row) {
               const modal = document.getElementById('editModal');
               const form = document.getElementById('editForm');
               form.innerHTML = '';

               const rowData = {};
               columns.forEach((col, index) => {
                   rowData[col] = row[index];
               });

               // Populate the form with current row data
               Object.keys(rowData).forEach(key => {
                   const div = document.createElement('div');
                   div.classList.add('flex', 'flex-col');
                   const label = document.createElement('label');
                   label.textContent = key;
                   label.classList.add('block', 'text-sm', 'font-medium', 'text-gray-700');
                   const input = document.createElement('input');
                   input.type = 'text';
                   input.value = rowData[key];
                   input.id = `edit-${key}`;
                   input.classList.add('mt-1', 'block', 'w-full', 'rounded-md', 'border-gray-300', 'shadow-sm', 'focus:border-blue-500', 'focus:ring-blue-500', 'sm:text-sm', 'p-2');
                   div.appendChild(label);
                   div.appendChild(input);
                   form.appendChild(div);
               });

               // Show the modal
               modal.classList.remove('hidden');
               modal.classList.add('visible');

               // Handle save button click
               document.getElementById('saveEdit').onclick = () => {
                   const updatedData = {};
                   Object.keys(rowData).forEach(key => {
                       updatedData[key] = document.getElementById(`edit-${key}`).value;
                   });

                   try {
                       const uniqueId = rowData[uniqueIdColumn];
                       if (!uniqueId) {
                           showMessage('无法修改行，未找到唯一的ID列。');
                           modal.classList.add('hidden');
                           return;
                       }

                       const setClause = Object.keys(updatedData).map(key => `"${key}" = ?`).join(', ');
                       const values = Object.values(updatedData);

                       const statement = db.prepare(`UPDATE "${tableName}" SET ${setClause} WHERE "${uniqueIdColumn}" = ?`);
                       statement.bind([...values, uniqueId]);
                       statement.step();
                       statement.free();

                       // Update the in-memory data
                       const originalIndex = fullData.findIndex(d => d[columns.findIndex(col => col === uniqueIdColumn)] === uniqueId);
                       if (originalIndex !== -1) {
                           const newRow = columns.map(col => updatedData[col]);
                           fullData[originalIndex] = newRow;
                       }

                       showMessage(`成功修改了 ID 为 ${uniqueId} 的行。请点击“保存数据库”下载更新后的文件。`);
                       filterAndRender();
                       modal.classList.add('hidden');
                   } catch (e) {
                       console.error("修改行时出错:", e);
                       showMessage(`修改行时出错: ${e.message}`);
                       modal.classList.add('hidden');
                   }
               };

               // Handle cancel button click
               document.getElementById('cancelEdit').onclick = () => {
                   modal.classList.add('hidden');
               };
           }
       </script>
   </body>
   </html>

)";

// 外部依赖
// typedef 和 struct
typedef QVector<DestinationPtr> DestinationList;

struct LogMessage {
    QString message; // 日志消息的文本内容
    Level level;     // 日志消息的级别（Trace, Debug, Info等）
};

// 一个在单独线程中执行的日志写入器
class LogWriterRunnable : public QRunnable
{
public:
    // 构造函数，接受一个 LoggerImpl 指针，以便访问日志器的数据
    LogWriterRunnable(LoggerImpl* impl);
    // 重写 run() 方法，这是线程的入口点
    void run() override;
private:
    LoggerImpl* m_impl; // 指向 LoggerImpl 实例的指针
};

// 包含所有日志数据和线程同步机制
class LoggerImpl
{
public:
    LoggerImpl();
    ~LoggerImpl();

    QVector<DestinationPtr> destinations; // 日志目的地列表，例如文件、控制台等
    Level logLevel;                   // 当前设置的日志级别
    bool includeTimestamp;            // 是否在日志中包含时间戳
    bool includeLogLevel;             // 是否在日志中包含日志级别
    QThreadPool threadPool;           // 用于运行日志写入线程的线程池
    QQueue<LogMessage> messageQueue;  // 待写入的日志消息队列
    QMutex queueMutex;                // 保护消息队列的互斥锁
    QWaitCondition queueWaitCondition; // 用于线程同步的等待条件
    std::atomic_bool stopSignal;      // 用于向日志写入线程发送停止信号
};

// -- LoggerImpl 实现 --
LoggerImpl::LoggerImpl() :
    logLevel(InfoLevel),
    includeTimestamp(true),
    includeLogLevel(true),
    stopSignal(false) // 初始化停止信号为 false
{
    // 设置线程池最大线程数为 1，确保只有一个日志写入线程在工作
    threadPool.setMaxThreadCount(1);
    // 启动日志写入线程
    threadPool.start(new LogWriterRunnable(this));
}

LoggerImpl::~LoggerImpl()
{
    // 在析构函数中设置停止信号为 true
    stopSignal = true;
    // 唤醒所有等待中的线程（虽然只有一个），以便其能够退出循环
    queueWaitCondition.wakeAll();
    // 等待线程池中的所有任务完成，确保日志写入线程已经安全退出
    threadPool.waitForDone();
    // 锁定互斥锁，清空消息队列，避免在析构时有消息遗留
    QMutexLocker locker(&queueMutex);
    messageQueue.clear();
}

// -- LogWriterRunnable 实现 --
LogWriterRunnable::LogWriterRunnable(LoggerImpl* impl) : m_impl(impl)
{
    // 确保 QRunnable 在任务完成后自动销毁
    setAutoDelete(true);
}

void LogWriterRunnable::run()
{
    // 线程主循环，只要停止信号为 false 就一直运行
    while (!m_impl->stopSignal) {
        // 锁定互斥锁，访问共享的消息队列
        m_impl->queueMutex.lock();
        // 如果消息队列为空，则进入等待状态
        if (m_impl->messageQueue.isEmpty()) {
            // 等待直到有新消息或超时（100毫秒）
            m_impl->queueWaitCondition.wait(&m_impl->queueMutex, 100);
        }
        // 再次检查队列是否为空（可能因超时而唤醒），如果为空则继续下一次循环
        if (m_impl->messageQueue.isEmpty()) {
            m_impl->queueMutex.unlock();
            continue;
        }

        // 从队列中取出一条消息
        LogMessage message = m_impl->messageQueue.dequeue();
        // 解锁互斥锁，让其他线程可以继续向队列添加消息
        m_impl->queueMutex.unlock();

        // 遍历所有日志目的地，并将消息写入
        for (const auto& dest : m_impl->destinations) {
            if (dest && dest->isValid()) {
                dest->write(message.message, message.level);
            }
        }
    }
}

// -- Logger 实现 --
// 创建 HTML 文件，用于日志输出（如果不存在的话）
void createHtmlFile() {
    QFile file(HTML_FILENAME);
    if (!file.exists()) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // 解决乱码的关键：设置 QTextStream 的编码为 UTF-8
            QTextStream out(&file);
            out.setCodec("UTF-8");

            out << HTML_CONTENT;
            file.close();
            qDebug() << "HTML file '" << HTML_FILENAME << "' created successfully.";
        } else {
            qWarning() << "Error: Could not create or open file '" << HTML_FILENAME << "'.";
        }
    } else {
        qDebug() << "File '" << HTML_FILENAME << "' already exists, no need to create.";
    }
}


// 获取 Logger 实例的单例方法
Logger& Logger::instance()
{
    // 锁定互斥锁以确保线程安全
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        // 如果实例不存在，则先创建 HTML 文件
        createHtmlFile();
        // 然后创建新的 Logger 实例
        s_instance = new Logger;
    }
    // 返回单例引用
    return *s_instance;
}

// 销毁 Logger 实例的单例方法
void Logger::destroyInstance()
{
    // 锁定互斥锁
    QMutexLocker locker(&s_instanceMutex);
    if (s_instance) {
        // 删除实例并将其指针置空
        delete s_instance;
        s_instance = nullptr;
    }
}

// Logger 构造函数
Logger::Logger() : d(new LoggerImpl) {}

// Logger 析构函数
Logger::~Logger()
{
    // 删除 LoggerImpl 实例
    delete d;
}

// 添加日志目的地
void Logger::addDestination(DestinationPtr destination)
{
    Q_ASSERT(destination.data());
    d->destinations.push_back(destination);
}

// 设置日志级别
void Logger::setLoggingLevel(Level newLevel)
{
    d->logLevel = newLevel;
}

// 获取当前日志级别
Level Logger::loggingLevel() const
{
    return d->logLevel;
}

// 设置是否包含时间戳
void Logger::setIncludeTimestamp(bool e)
{
    d->includeTimestamp = e;
}

// 获取是否包含时间戳
bool Logger::includeTimestamp() const
{
    return d->includeTimestamp;
}

// 设置是否包含日志级别
void Logger::setIncludeLogLevel(bool l)
{
    d->includeLogLevel = l;
}

// 获取是否包含日志级别
bool Logger::includeLogLevel() const
{
    return d->includeLogLevel;
}

// -- Logger 补充实现 --
// 刷新日志：等待消息队列中的所有消息被处理
void Logger::flush()
{
    // 锁定互斥锁以检查队列状态
    QMutexLocker locker(&d->queueMutex);
    // 循环直到消息队列为空
    while (!d->messageQueue.isEmpty()) {
        // 暂时解锁，让日志写入线程有机会处理消息
        locker.unlock();
        // 睡眠 50 毫秒
        QThread::msleep(50);
        // 重新锁定
        locker.relock();
    }
}

// Logger::Helper 的析构函数
Logger::Helper::~Helper()
{
    try {
        QString finalMessage;
        // 静态数组，用于将日志级别转换为字符串
        static const char* const level_string[] = {
            "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "OFF"
        };

        // 获取并处理日志消息
        finalMessage += buffer.trimmed();

        // 锁定互斥锁，将消息添加到队列
        QMutexLocker locker(&Logger::instance().d->queueMutex);
        Logger::instance().d->messageQueue.enqueue({ finalMessage, level });
        // 唤醒日志写入线程，通知其有新消息需要处理
        Logger::instance().d->queueWaitCondition.wakeOne();

    } catch(std::exception&) {
        // 捕获异常，如果析构函数中发生异常，则断言失败
        Q_ASSERT(!"exception in logger helper destructor");
    }
}

} // end namespace
