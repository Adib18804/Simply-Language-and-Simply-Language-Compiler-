const express = require('express');
const router = express.Router();
const { compileAndRun } = require('../services/compilerService');

router.post('/compile', async (req, res) => {
    try {
        const { code, run } = req.body;
        if (typeof code !== 'string') {
            return res.status(400).json({ success: false, errors: [{ message: 'Missing "code" in request body.' }] });
        }
        const shouldRun = run === true;
        const result = await compileAndRun(code, shouldRun);
        return res.json(result);
    } catch (err) {
        console.error('API /api/compile error:', err);
        return res.status(500).json({
            success: false,
            errors: [{ type: 'Server Error', message: err.message || String(err) }]
        });
    }
});

module.exports = router;
