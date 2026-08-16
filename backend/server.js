const express = require('express');
const cors = require('cors');
const path = require('path');
const compileRoutes = require('./routes/compile');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json({ limit: '1mb' }));
app.use(express.urlencoded({ extended: true }));

app.use(express.static(path.join(__dirname, '..', 'frontend')));

app.use('/api', compileRoutes);

app.get('/health', (req, res) => {
    res.json({ status: 'ok', service: 'Simply Compiler Backend' });
});

app.listen(PORT, () => {
    console.log(`\n========================================`);
    console.log(`   Simply Compiler Web IDE`);
    console.log(`========================================`);
    console.log(`   Running on:  http://localhost:${PORT}`);
    console.log(`   Backend API: http://localhost:${PORT}/api/compile`);
    console.log(`   Health:      http://localhost:${PORT}/health`);
    console.log(`========================================\n`);
});
